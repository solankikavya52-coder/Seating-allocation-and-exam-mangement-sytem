#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <memory>
#include <iostream>

using namespace sql;

std::unique_ptr<Connection> createConnection() {
    Driver* driver = get_driver_instance();

    Connection* con = driver->connect("tcp://127.0.0.1:3306", "root", "");
    con->setSchema("exam_system");

    return std::unique_ptr<Connection>(con);
}

int main() {
    try {
        auto db = createConnection();
        std::cout << "Connected to exam_system database";
    } catch (SQLException& e) {
        std::cerr << "Connection failed: " << e.what();
    }

    return 0;
}