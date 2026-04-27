#ifndef ENV_FILE_PATH
#  define ENV_FILE_PATH "../.env"
#endif

#include "db/MySQLConnection.h"
#include "db/MySQLExecutor.h"
#include "db/DictionaryRepository.h"
#include "config/Env.h"
#include <iostream>
#include <cassert>

static int passed = 0;
static int failed = 0;

#define CHECK(label, cond) \
    do { \
        if (cond) { std::cout << "[PASS] " << (label) << "\n"; ++passed; } \
        else      { std::cout << "[FAIL] " << (label) << "\n"; ++failed; } \
    } while(0)

int main() {
    auto env = loadEnv(ENV_FILE_PATH);

    MySQLConnection& conn = MySQLConnection::getInstance();
    try {
        conn.connect(
            env["MYSQL_HOST"] + ":" + env["MYSQL_PORT"],
            env["MYSQL_ROOT"],
            env["MYSQL_ROOT_PASSWORD"],
            env["MYSQL_DATABASE"]
        );
    } catch (sql::SQLException& e) {
        std::cerr << "Connection failed: " << e.what() << "\n";
        return 1;
    }

    DictionaryRepository repo(&conn);
    const std::string table = "dishes";

    // ── getAll: baseline row count ──────────────────────────────────────────
    auto before = repo.getAll(table);
    CHECK("getAll returns rows", !before.empty());
    std::cout << "  rows before test: " << before.size() << "\n";

    // ── add ─────────────────────────────────────────────────────────────────
    DictionaryEntry newEntry{"__test_word__", 42, 0};
    repo.add(newEntry, table);

    auto after_add = repo.getAll(table);
    CHECK("add increases row count by 1", after_add.size() == before.size() + 1);

    // find the inserted row
    int insertedId = -1;
    for (const auto& e : after_add) {
        if (e.word == "__test_word__" && e.frequency == 42) {
            insertedId = e.id;
            break;
        }
    }
    CHECK("add inserts correct word and frequency", insertedId != -1);

    // ── getById ─────────────────────────────────────────────────────────────
    if (insertedId != -1) {
        auto fetched = repo.getById(insertedId, table);
        CHECK("getById returns correct word",      fetched.word      == "__test_word__");
        CHECK("getById returns correct frequency", fetched.frequency == 42);
        CHECK("getById returns correct id",        fetched.id        == insertedId);
    }

    // ── update ──────────────────────────────────────────────────────────────
    if (insertedId != -1) {
        DictionaryEntry updated{"__test_word_updated__", 99, insertedId};
        repo.update(updated, table);

        auto fetched = repo.getById(insertedId, table);
        CHECK("update changes word",      fetched.word      == "__test_word_updated__");
        CHECK("update changes frequency", fetched.frequency == 99);
    }

    // ── remove ──────────────────────────────────────────────────────────────
    if (insertedId != -1) {
        repo.remove(insertedId, table);

        auto after_remove = repo.getAll(table);
        CHECK("remove decreases row count back to original", after_remove.size() == before.size());

        auto fetched = repo.getById(insertedId, table);
        CHECK("remove makes getById return empty entry", fetched.word.empty() && fetched.id == 0);
    }

    std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
