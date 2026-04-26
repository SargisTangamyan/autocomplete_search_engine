#pragma once
#include "IDBConnection.h"
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <memory>

class MySQLConnection : public IDBConnection {
private:
    std::unique_ptr<sql::Connection> conn;
    sql::Driver* driver;

public:
    MySQLConnection() {
        driver = get_driver_instance();
    }

    void connect(const std::string& host,
                 const std::string& user,
                 const std::string& password,
                 const std::string& database) override {
        conn.reset(driver->connect(host, user, password));
        conn->setSchema(database);
    }

    sql::Connection* getConnection() override {
        return conn.get();
    }
};