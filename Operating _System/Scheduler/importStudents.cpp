#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <OpenXLSX.hpp>

using namespace std;
using namespace sql;
using namespace OpenXLSX;

struct FailedRecord {
    string roll_no;
    string reason;
};

struct ImportResult {
    int total = 0;
    int inserted = 0;
    int duplicates = 0;
    int failed = 0;
    vector<string> insertedRollNos;
    vector<string> duplicateRollNos;
    vector<FailedRecord> failedRecords;
};

string trim(string s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string getCellValue(XLWorksheet& sheet, int row, int col) {
    auto cell = sheet.cell(XLCellReference(row, col));
    auto value = cell.value();

    if (value.type() == XLValueType::Integer) {
        return to_string(value.get<int64_t>());
    }

    if (value.type() == XLValueType::Float) {
        return to_string(value.get<double>());
    }

    if (value.type() == XLValueType::Boolean) {
        return value.get<bool>() ? "1" : "0";
    }

    if (value.type() == XLValueType::String) {
        return value.get<string>();
    }

    return "";
}

int getBranchId(Connection* db, const string& branchName) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "SELECT branch_id FROM BRANCH WHERE branch_name = ?"
        )
    );

    stmt->setString(1, branchName);

    unique_ptr<ResultSet> result(stmt->executeQuery());

    if (result->next()) {
        return result->getInt("branch_id");
    }

    unique_ptr<PreparedStatement> insertStmt(
        db->prepareStatement(
            "INSERT INTO BRANCH(branch_name) VALUES (?)"
        )
    );

    insertStmt->setString(1, branchName);
    insertStmt->executeUpdate();

    unique_ptr<PreparedStatement> idStmt(
        db->prepareStatement("SELECT LAST_INSERT_ID()")
    );

    unique_ptr<ResultSet> idResult(idStmt->executeQuery());
    idResult->next();

    return idResult->getInt(1);
}

int getSemesterId(Connection* db, int semesterNo) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "SELECT semester_id FROM SEMESTER WHERE semester_no = ?"
        )
    );

    stmt->setInt(1, semesterNo);

    unique_ptr<ResultSet> result(stmt->executeQuery());

    if (result->next()) {
        return result->getInt("semester_id");
    }

    unique_ptr<PreparedStatement> insertStmt(
        db->prepareStatement(
            "INSERT INTO SEMESTER(semester_no) VALUES (?)"
        )
    );

    insertStmt->setInt(1, semesterNo);
    insertStmt->executeUpdate();

    unique_ptr<PreparedStatement> idStmt(
        db->prepareStatement("SELECT LAST_INSERT_ID()")
    );

    unique_ptr<ResultSet> idResult(idStmt->executeQuery());
    idResult->next();

    return idResult->getInt(1);
}

bool checkDuplicateRollNo(Connection* db, const string& rollNo) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "SELECT student_id FROM STUDENT WHERE roll_no = ?"
        )
    );

    stmt->setString(1, rollNo);

    unique_ptr<ResultSet> result(stmt->executeQuery());

    return result->next();
}

void insertStudent(
    Connection* db,
    const string& rollNo,
    const string& name,
    const string& email,
    int branchId,
    int semesterId
) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "INSERT INTO STUDENT "
            "(roll_no, name, email, branch_id, semester_id) "
            "VALUES (?, ?, ?, ?, ?)"
        )
    );

    stmt->setString(1, rollNo);
    stmt->setString(2, name);

    if (email.empty()) {
        stmt->setNull(3, DataType::VARCHAR);
    } else {
        stmt->setString(3, email);
    }

    stmt->setInt(4, branchId);
    stmt->setInt(5, semesterId);

    stmt->executeUpdate();
}

ImportResult importStudents(const string& filePath) {
    ImportResult result;
    XLDocument workbook;

    workbook.open(filePath);

    auto sheet = workbook.workbook().worksheet(
        workbook.workbook().worksheetNames()[0]
    );

    int lastRow = sheet.lastCell().row();
    int lastCol = sheet.lastCell().column();

    if (lastRow < 1) {
        workbook.close();
        return result;
    }

    vector<string> headers;

    for (int col = 1; col <= lastCol; col++) {
        headers.push_back(trim(getCellValue(sheet, 1, col)));
    }

    auto findColumn = [&](const string& name) -> int {
        for (int i = 0; i < (int)headers.size(); i++) {
            if (headers[i] == name) {
                return i + 1;
            }
        }
        return 0;
    };

    int rollCol = findColumn("roll_no");
    int nameCol = findColumn("name");
    int emailCol = findColumn("email");
    int branchCol = findColumn("branch");
    int semesterCol = findColumn("semester");

    if (!rollCol || !nameCol || !branchCol || !semesterCol) {
        workbook.close();
        throw runtime_error("Required Excel columns are missing");
    }

    Driver* driver = get_driver_instance();

    unique_ptr<Connection> db(
        driver->connect("tcp://127.0.0.1:3306", "root", "")
    );

    db->setSchema("exam_system");

    for (int row = 2; row <= lastRow; row++) {
        result.total++;

        string rollNo = trim(getCellValue(sheet, row, rollCol));
        string name = trim(getCellValue(sheet, row, nameCol));
        string email;

        if (emailCol) {
            email = trim(getCellValue(sheet, row, emailCol));
        }

        string branch = trim(getCellValue(sheet, row, branchCol));
        string semesterText = trim(getCellValue(sheet, row, semesterCol));

        int semester = 0;

        try {
            if (semesterText.empty()) {
                throw runtime_error("Missing required data");
            }

            size_t pos = 0;
            semester = stoi(semesterText, &pos);

            if (pos != semesterText.size() ||
                rollNo.empty() ||
                name.empty() ||
                branch.empty() ||
                semester <= 0) {
                throw runtime_error("Missing or invalid required data");
            }
        } catch (const exception& e) {
            result.failed++;
            result.failedRecords.push_back({rollNo, e.what()});
            continue;
        }

        try {
            if (checkDuplicateRollNo(db.get(), rollNo)) {
                result.duplicates++;
                result.duplicateRollNos.push_back(rollNo);
                continue;
            }

            int branchId = getBranchId(db.get(), branch);
            int semesterId = getSemesterId(db.get(), semester);

            insertStudent(
                db.get(),
                rollNo,
                name,
                email,
                branchId,
                semesterId
            );

            result.inserted++;
            result.insertedRollNos.push_back(rollNo);
        } catch (const exception& e) {
            result.failed++;
            result.failedRecords.push_back({rollNo, e.what()});
        }
    }

    workbook.close();

    return result;
}

int main() {
    try {
        ImportResult result = importStudents("students.xlsx");

        cout << "Total: " << result.total << '\n';
        cout << "Inserted: " << result.inserted << '\n';
        cout << "Duplicates: " << result.duplicates << '\n';
        cout << "Failed: " << result.failed << '\n';

        cout << "\nInserted roll numbers:\n";
        for (const string& rollNo : result.insertedRollNos) {
            cout << rollNo << '\n';
        }

        cout << "\nDuplicate roll numbers:\n";
        for (const string& rollNo : result.duplicateRollNos) {
            cout << rollNo << '\n';
        }

        cout << "\nFailed records:\n";
        for (const auto& record : result.failedRecords) {
            cout << record.roll_no << " : "
                 << record.reason << '\n';
        }
    } catch (const exception& e) {
        cerr << "Import failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}