#pragma once
#include <string>
#include <cppconn/connection.h>

class IDBConnection {
public:
    virtual ~IDBConnection() {}
    virtual void connect(const std::string& host,
                         const std::string& user,
                         const std::string& password,
                         const std::string& database) = 0;
                         
    virtual sql::Connection* getConnection() = 0;
};