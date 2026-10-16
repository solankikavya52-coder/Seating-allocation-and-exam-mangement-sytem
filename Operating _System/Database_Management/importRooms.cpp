#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <limits>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
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

string normalizeHeader(string s) {
    s = trim(s);
    transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

string getCellValue(XLWorksheet& sheet, int row, int col) {
    auto value = sheet.cell(XLCellReference(row, col)).value();

    if (value.type() == XLValueType::Integer) {
        return to_string(value.get<int64_t>());
    }
    if (value.type() == XLValueType::Float) {
        double number = value.get<double>();
        if (isfinite(number) && floor(number) == number &&
            number >= static_cast<double>(numeric_limits<long long>::min()) &&
            number <= static_cast<double>(numeric_limits<long long>::max())) {
            return to_string(static_cast<long long>(number));
        }
        ostringstream output;
        output << setprecision(15) << number;
        return output.str();
    }
    if (value.type() == XLValueType::Boolean) {
        return value.get<bool>() ? "1" : "0";
    }
    if (value.type() == XLValueType::String) {
        return value.get<string>();
    }
    return "";
}

ImportResult importRooms(const string& filePath) {
    ImportResult result;
    XLDocument workbook;
    workbook.open(filePath);

    try {
        auto sheet = workbook.workbook().worksheet(
            workbook.workbook().worksheetNames()[0]
        );

        int lastRow = sheet.lastCell().row();
        int lastCol = sheet.lastCell().column();

        if (lastRow < 1 || lastCol < 1) {
            workbook.close();
            return result;
        }

        vector<string> headers;
        for (int col = 1; col <= lastCol; ++col) {
            headers.push_back(normalizeHeader(getCellValue(sheet, 1, col)));
        }

        auto findColumn = [&](const string& name) -> int {
            for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
                if (headers[i] == name) {
                    return i + 1;
                }
            }
            return 0;
        };

        int roomCol = findColumn("room_no");
        int capacityCol = findColumn("capacity");

        if (!roomCol || !capacityCol) {
            throw runtime_error("Required Excel columns room_no and capacity are missing");
        }

        Driver* driver = get_driver_instance();
        unique_ptr<Connection> db(
            driver->connect("tcp://127.0.0.1:3306", "root", "")
        );
        db->setSchema("exam_system");

        unique_ptr<PreparedStatement> checkStmt(
            db->prepareStatement("SELECT room_id FROM ROOM WHERE room_no = ?")
        );
        unique_ptr<PreparedStatement> insertStmt(
            db->prepareStatement("INSERT INTO ROOM (room_no, capacity) VALUES (?, ?)")
        );

        for (int row = 2; row <= lastRow; ++row) {
            string roomNo = trim(getCellValue(sheet, row, roomCol));
            string capacityText = trim(getCellValue(sheet, row, capacityCol));
            result.total++;

            try {
                if (roomNo.empty() || capacityText.empty()) {
                    throw runtime_error("Room number and capacity are required");
                }

                size_t pos = 0;
                int capacity = stoi(capacityText, &pos);
                if (pos != capacityText.size() || capacity <= 0) {
                    throw runtime_error("Capacity must be a positive integer");
                }

                checkStmt->setString(1, roomNo);
                unique_ptr<ResultSet> existing(checkStmt->executeQuery());
                if (existing->next()) {
                    result.duplicates++;
                    result.duplicateRooms.push_back(roomNo);
                    continue;
                }

                insertStmt->setString(1, roomNo);
                insertStmt->setInt(2, capacity);
                insertStmt->executeUpdate();

                result.inserted++;
                result.insertedRooms.push_back(roomNo);
            } catch (const exception& e) {
                result.failed++;
                result.failedRecords.push_back({roomNo, e.what()});
            }
        }

        workbook.close();
        return result;
    } catch (...) {
        workbook.close();
        throw;
    }
}

int main() {
    try {
        ImportResult result = importRooms("rooms.xlsx");

        cout << "Total: " << result.total << '\n';
        cout << "Inserted: " << result.inserted << '\n';
        cout << "Duplicates: " << result.duplicates << '\n';
        cout << "Failed: " << result.failed << '\n';

        cout << "\\nInserted rooms:\\n";
        for (const string& room : result.insertedRooms) {
            cout << room << '\n';
        }

        cout << "\\nDuplicate rooms:\\n";
        for (const string& room : result.duplicateRooms) {
            cout << room << '\n';
        }

        cout << "\\nFailed records:\\n";
        for (const auto& record : result.failedRecords) {
            cout << record.room_no << " : " << record.reason << '\n';
        }
    } catch (const exception& e) {
        cerr << "Import failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
