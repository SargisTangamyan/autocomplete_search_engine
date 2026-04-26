#pragma once
#include "IDBConnection.h"
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <memory>

class MySQLConnection : public IDBConnection {
private:
    std::unique_ptr<sql::Connection> conn;
    sql::Driver* driver;

    // Private constructor for singleton
    MySQLConnection() {
        driver = get_driver_instance();
    }

public:
    // Prevent copying — only one instance should exist
    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&) = delete;

    static MySQLConnection& getInstance() {
        static MySQLConnection instance;
        return instance;
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

