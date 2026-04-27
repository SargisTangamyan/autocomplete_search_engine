#pragma once
#include "DictionaryEntry.h"
#include <vector>
#include <string>

class IDictionaryRepository {
public:
    virtual ~IDictionaryRepository() {}
    virtual void add(const DictionaryEntry& entry, const std::string& tableName) = 0;
    virtual void remove(int id, const std::string& tableName) = 0;
    virtual void update(const DictionaryEntry& entry, const std::string& tableName) = 0;
    virtual std::vector<DictionaryEntry> getAll(const std::string& tableName) = 0;
    virtual DictionaryEntry getById(int id, const std::string& tableName) = 0;
};
