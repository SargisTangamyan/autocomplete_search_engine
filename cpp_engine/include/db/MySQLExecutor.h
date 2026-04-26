#pragma once
#include "IDBExecutor.h"
#include "IDBConnection.h"
#include <cppconn/statement.h>
#include <memory>

class MySQLExecutor : public IDBExecutor {
private:
    IDBConnection* dbConnection;

public:
    MySQLExecutor(IDBConnection* conn) : dbConnection(conn) {}

    void execute(const std::string& query) override {
        std::unique_ptr<sql::Statement> stmt(
            dbConnection->getConnection()->createStatement()
        );
        stmt->execute(query);
    }
};