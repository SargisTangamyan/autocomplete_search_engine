#pragma once
#include <string>

class IDBExecutor {
public:
    virtual ~IDBExecutor() {}
    virtual void execute(const std::string& query) = 0;
};