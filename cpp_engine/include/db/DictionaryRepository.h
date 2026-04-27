#pragma once
#include "IDictionaryRepository.h"
#include "IDBConnection.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>

class DictionaryRepository : public IDictionaryRepository {
private:
    IDBConnection* dbConnection;

public:
    explicit DictionaryRepository(IDBConnection* conn)
        : dbConnection(conn) {}

    void add(const DictionaryEntry& entry, const std::string& tableName) override {
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                dbConnection->getConnection()->prepareStatement(
                    "INSERT INTO " + tableName + " (name, frequency) VALUES (?, ?)"
                )
            );
            stmt->setString(1, entry.word);
            stmt->setInt(2, entry.frequency);
            stmt->executeUpdate();
        } catch (sql::SQLException& e) {
            std::cerr << "[DB] add error: " << e.what() << "\n";
        }
    }

    void remove(int id, const std::string& tableName) override {
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                dbConnection->getConnection()->prepareStatement(
                    "DELETE FROM " + tableName + " WHERE id = ?"
                )
            );
            stmt->setInt(1, id);
            stmt->executeUpdate();
        } catch (sql::SQLException& e) {
            std::cerr << "[DB] remove error: " << e.what() << "\n";
        }
    }

    void update(const DictionaryEntry& entry, const std::string& tableName) override {
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                dbConnection->getConnection()->prepareStatement(
                    "UPDATE " + tableName + " SET name = ?, frequency = ? WHERE id = ?"
                )
            );
            stmt->setString(1, entry.word);
            stmt->setInt(2, entry.frequency);
            stmt->setInt(3, entry.id);
            stmt->executeUpdate();
        } catch (sql::SQLException& e) {
            std::cerr << "[DB] update error: " << e.what() << "\n";
        }
    }

    std::vector<DictionaryEntry> getAll(const std::string& tableName) override {
        std::vector<DictionaryEntry> results;
        try {
            std::unique_ptr<sql::Statement> stmt(
                dbConnection->getConnection()->createStatement()
            );
            std::unique_ptr<sql::ResultSet> res(
                stmt->executeQuery("SELECT id, name, frequency FROM " + tableName)
            );
            while (res->next()) {
                DictionaryEntry entry;
                entry.id        = res->getInt("id");
                entry.word      = res->getString("name");
                entry.frequency = res->getInt("frequency");
                results.push_back(entry);
            }
        } catch (sql::SQLException& e) {
            std::cerr << "[DB] getAll error: " << e.what() << "\n";
        }
        return results;
    }

    DictionaryEntry getById(int id, const std::string& tableName) override {
        DictionaryEntry entry{};
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                dbConnection->getConnection()->prepareStatement(
                    "SELECT id, name, frequency FROM " + tableName + " WHERE id = ?"
                )
            );
            stmt->setInt(1, id);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res->next()) {
                entry.id        = res->getInt("id");
                entry.word      = res->getString("name");
                entry.frequency = res->getInt("frequency");
            }
        } catch (sql::SQLException& e) {
            std::cerr << "[DB] getById error: " << e.what() << "\n";
        }
        return entry;
    }
};
