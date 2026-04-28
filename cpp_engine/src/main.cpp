/*
 * Autocomplete Engine – CLI entry point
 *
 * Two operating modes:
 *
 *   Single-shot  (ideal for Laravel shell_exec / proc_open one-shot calls):
 *     ./engine "prefix"             – normal top-5 completions, JSON to stdout
 *     ./engine --fuzzy "prefix"     – fuzzy completions (edit-distance ≤ 1)
 *     ./engine --top 10 "prefix"    – request up to 10 results
 *
 *   REPL / pipe  (ideal for Laravel proc_open persistent process):
 *     ./engine                      – reads lines from stdin, writes JSON per line
 *     In REPL mode each line is treated as a prefix query.
 *     Special commands:
 *       ?<prefix>   fuzzy query
 *       +<word>     insert word
 *       -<word>     remove word
 *       :stats      word count
 *       :quit       exit
 *
 * All output lines are newline-terminated JSON so they are trivially
 * parseable by json_decode() in PHP / Laravel.
 *
 * Complexity reference:
 *   insert   O(N)          N = word length
 *   search   O(N)
 *   delete   O(N)
 *   prefix   O(P + S·log k)  P = prefix length, S = subtree size, k = top_n
 *   fuzzy    O(|Σ|^(P+1) · S)  bounded by alphabet and edit-distance budget
 */

#include "db/MySQLConnection.h"
#include "search/Autocomplete.h"
#include "db/DictionaryRepository.h"
#include "config/Env.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <stdexcept>

// ─────────────────────────────────────────────
//  Minimal JSON helpers  (no external library)
// ─────────────────────────────────────────────
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += static_cast<char>(c);
        }
    }
    return out;
}

static std::string make_results_json(const std::string& query,
                                     bool fuzzy,
                                     const std::vector<Completion>& results,
                                     double elapsed_ms) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    o << "{\"query\":\"" << json_escape(query) << "\""
      << ",\"fuzzy\":"   << (fuzzy ? "true" : "false")
      << ",\"elapsed_ms\":" << elapsed_ms
      << ",\"count\":"   << results.size()
      << ",\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i) o << ",";
        o << "{\"id\":"    << results[i].id
          << ",\"word\":\"" << json_escape(results[i].word)
          << "\",\"score\":" << results[i].score << "}";
    }
    o << "]}";
    return o.str();
}

// ─────────────────────────────────────────────
//  Run one prefix query and print JSON
// ─────────────────────────────────────────────
static void run_query(AutocompleteEngine& engine,
                      const std::string& prefix,
                      bool fuzzy) {
    auto t0 = std::chrono::high_resolution_clock::now();
    auto results = fuzzy ? engine.fuzzy_query(prefix) : engine.query(prefix);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << make_results_json(prefix, fuzzy, results, ms) << "\n";
    std::cout.flush();
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {

    // ── Parse CLI flags ───────────────────────
    bool        fuzzy_flag  = false;
    int         top_n       = 5;
    std::string query_arg;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--fuzzy" || a == "-f") {
            fuzzy_flag = true;
        } else if ((a == "--top" || a == "-n") && i + 1 < argc) {
            try { top_n = std::stoi(argv[++i]); } catch (...) {}
            top_n = std::max(1, std::min(top_n, 20));
        } else {
            query_arg = a;
        }
    }

    // ── Load environment ──────────────────────
    // Tries ../.env (relative to build dir) first; override with
    // SEARCH_ENGINE_ENV environment variable for production deployments.
    const char* env_path_override = std::getenv("SEARCH_ENGINE_ENV");
    std::string env_path = env_path_override ? env_path_override : "../.env";
    auto env = loadEnv(env_path);

    // ── Database connection ───────────────────
    MySQLConnection& conn = MySQLConnection::getInstance();
    bool db_ok = false;
    try {
        std::string url = "tcp://" + env["MYSQL_HOST"] + ":" + env["MYSQL_PORT"];
        conn.connect(url, env["MYSQL_ROOT"], env["MYSQL_ROOT_PASSWORD"], env["MYSQL_DATABASE"]);
        db_ok = true;
    } catch (const std::exception& e) {
        std::cerr << "[engine] DB connection failed: " << e.what()
                  << " — falling back to dataset file\n";
    }

    // ── Build autocomplete engine ─────────────
    AutocompleteEngine engine(top_n, 50, 200);

    if (db_ok) {
        DictionaryRepository repo(&conn);
        engine.load_from_db(repo, "dishes");
    }

    // Fallback: load from flat file when DB is unavailable or empty
    if (engine.word_count() == 0) {
        // Try paths relative to both cwd and binary location
        for (const auto& p : {"data/dataset.txt", "../data/dataset.txt"}) {
            if (engine.load_from_file(p) && engine.word_count() > 0) break;
        }
    }

    std::cerr << "[engine] Ready — " << engine.word_count() << " words loaded\n";

    // ── Single-shot mode ──────────────────────
    if (!query_arg.empty()) {
        run_query(engine, query_arg, fuzzy_flag);
        return 0;
    }

    // ── REPL / pipe mode ──────────────────────
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (line == ":quit" || line == ":q") break;

        if (line == ":stats") {
            std::cout << "{\"words\":" << engine.word_count() << "}\n";
            std::cout.flush();
        } else if (line[0] == '+') {
            std::string word = line.substr(1);
            engine.load_word(word);
            std::cout << "{\"status\":\"inserted\",\"word\":\""
                      << json_escape(word) << "\"}\n";
            std::cout.flush();
        } else if (line[0] == '-') {
            std::string word = line.substr(1);
            engine.remove_word(word);
            std::cout << "{\"status\":\"removed\",\"word\":\""
                      << json_escape(word) << "\"}\n";
            std::cout.flush();
        } else if (line[0] == '?') {
            run_query(engine, line.substr(1), true);
        } else {
            run_query(engine, line, fuzzy_flag);
        }
    }

    return 0;
}
