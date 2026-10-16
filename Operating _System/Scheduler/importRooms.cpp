#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
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
    string room_no;
    string reason;
};

struct ImportResult {
    int total = 0;
    int inserted = 0;
    int duplicates = 0;
    int failed = 0;
    vector<string> insertedRooms;
    vector<string> duplicateRooms;
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

bool checkRoom(Connection* db, const string& roomNo) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "SELECT room_id FROM ROOM WHERE room_no = ?"
        )
    );

    stmt->setString(1, roomNo);

    unique_ptr<ResultSet> result(stmt->executeQuery());

    return result->next();
}

void insertRoom(Connection* db, const string& roomNo, int capacity) {
    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "INSERT INTO ROOM (room_no, capacity) VALUES (?, ?)"
        )
    );

    stmt->setString(1, roomNo);
    stmt->setInt(2, capacity);
    stmt->executeUpdate();
}

ImportResult importRooms(const string& filePath) {
    ImportResult result;
    XLDocument workbook;

    workbook.open(filePath);

    auto sheet = workbook.workbook().worksheet(
        workbook.workbook().worksheetNames()[0]
    );

    auto rows = sheet.range(
        "A1:" + XLCellReference(
            sheet.lastCell().row(),
            sheet.lastCell().column()
        ).address()
    );

    vector<string> headers;
    bool firstRow = true;

    Driver* driver = get_driver_instance();

    unique_ptr<Connection> db(
        driver->connect("tcp://127.0.0.1:3306", "root", "")
    );

    db->setSchema("exam_system");

    for (auto row : rows.rows()) {
        if (firstRow) {
            for (auto cell : row) {
                headers.push_back(trim(cell.value().get<string>()));
            }
            firstRow = false;
            continue;
        }

        result.total++;

        string roomNo;
        string capacityText;

        for (size_t i = 0; i < headers.size(); i++) {
            string value = trim(row[i].value().get<string>());

            if (headers[i] == "room_no") {
                roomNo = value;
            } else if (headers[i] == "capacity") {
                capacityText = value;
            }
        }

        roomNo = trim(roomNo);

        try {
            if (roomNo.empty() || capacityText.empty()) {
                result.failed++;
                result.failedRecords.push_back({
                    roomNo, "Invalid room data"
                });
                continue;
            }

            size_t pos = 0;
            int capacity = stoi(capacityText, &pos);

            if (pos != capacityText.size() || capacity <= 0) {
                result.failed++;
                result.failedRecords.push_back({
                    roomNo, "Invalid room data"
                });
                continue;
            }

            if (checkRoom(db.get(), roomNo)) {
                result.duplicates++;
                result.duplicateRooms.push_back(roomNo);
                continue;
            }

            insertRoom(db.get(), roomNo, capacity);

            result.inserted++;
            result.insertedRooms.push_back(roomNo);
        } catch (const exception& e) {
            result.failed++;
            result.failedRecords.push_back({
                roomNo, e.what()
            });
        }
    }

    workbook.close();

    return result;
}

int main() {
    try {
        ImportResult result = importRooms("rooms.xlsx");

        cout << "Total: " << result.total << '\n';
        cout << "Inserted: " << result.inserted << '\n';
        cout << "Duplicates: " << result.duplicates << '\n';
        cout << "Failed: " << result.failed << '\n';

        cout << "\nInserted rooms:\n";
        for (const string& room : result.insertedRooms) {
            cout << room << '\n';
        }

        cout << "\nDuplicate rooms:\n";
        for (const string& room : result.duplicateRooms) {
            cout << room << '\n';
        }

        cout << "\nFailed records:\n";
        for (const auto& record : result.failedRecords) {
            cout << record.room_no << " : "
                 << record.reason << '\n';
        }
    } catch (const exception& e) {
        cerr << "Import failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}