cmake_minimum_required(VERSION 3.16)
project(autocomplete_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include directories ────────────────────────────────────────────────────
#    Mirrors your -I./include flag
include_directories(include)

# ── Source files ───────────────────────────────────────────────────────────
set(ENGINE_SOURCES
    src/Trie.cpp
    src/Autocomplete.cpp
    src/config/Env.cpp          # your existing DB config
)

# ── Main binary ────────────────────────────────────────────────────────────
#    Mirrors: g++ src/main.cpp src/config/Env.cpp -o engine -I./include -lmysqlcppconn
add_executable(engine
    src/main.cpp
    ${ENGINE_SOURCES}
)
target_link_libraries(engine mysqlcppconn)

# ── Benchmark binary ───────────────────────────────────────────────────────
add_executable(benchmark
    src/Benchmark.cpp
    ${ENGINE_SOURCES}
)
target_link_libraries(benchmark mysqlcppconn)

# ── Compiler flags ─────────────────────────────────────────────────────────
foreach(target engine benchmark)
    target_compile_options(${target} PRIVATE -Wall -Wextra -O2)
endforeach()