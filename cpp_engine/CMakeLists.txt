cmake_minimum_required(VERSION 3.16)
project(search_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Sources ──────────────────────────────────────────────────────────────────
add_executable(search_engine
    src/main.cpp
    src/config/Env.cpp
    src/Trie.cpp
)

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(search_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/include/cppconn          # MySQL Connector/C++ headers
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(search_engine PRIVATE mysqlcppconn)

# ── Compiler warnings ─────────────────────────────────────────────────────────
target_compile_options(search_engine PRIVATE -Wall -Wextra -Wpedantic)

# ── Absolute path to .env baked in at compile time ───────────────────────────
# Avoids any working-directory sensitivity when running the binary.
target_compile_definitions(search_engine PRIVATE
    ENV_FILE_PATH="${CMAKE_SOURCE_DIR}/../.env"
)

# ── Convenience target: build + run ──────────────────────────────────────────
add_custom_target(run
    COMMAND search_engine
    DEPENDS search_engine
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running search_engine..."
)
