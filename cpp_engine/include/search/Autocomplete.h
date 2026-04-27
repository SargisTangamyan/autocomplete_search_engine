#pragma once
#include "Trie.h"
#include "Recency_cache.h"
#include <string>
#include <vector>
#include "db/IDBConnection.h"
#include "db/DictionaryRepository.h"

// ─────────────────────────────────────────────
//  AutocompleteEngine
//  Thin orchestration layer that glues together:
//    • the Trie (prefix retrieval)
//    • the RecencyCache (search-history boost)
//  Your DB loader calls load_word() / load_words()
//  and then the engine is ready to serve queries.
// ─────────────────────────────────────────────
class AutocompleteEngine {
public:
    // top_n      – how many suggestions to return
    // cache_size – how many recent queries to remember
    // recency_bonus – score boost for recently queried terms
    explicit AutocompleteEngine(int top_n        = 5,
                                int cache_size   = 20,
                                int recency_bonus = 100);

    // ── Loading ──────────────────────────────
    void load_word (const std::string& word, int frequency = 1, int id = 0);
    void load_words(const std::vector<std::string>& words);

    // Load from a plain-text file (one word / phrase per line)
    bool load_from_file(const std::string& filepath);
    bool load_from_db(DictionaryRepository& dictRepo, const std::string& tableName);

    // Clear all words and recency history (use before reload)
    void clear();

    // ── Querying ─────────────────────────────
    // Returns top-N completions for the given prefix.
    // Records the prefix in the recency cache.
    // Returns {} if prefix is not found.
    std::vector<Completion>
        query(const std::string& prefix);

    // Fuzzy query (edit distance ≤ 1)
    std::vector<Completion>
        fuzzy_query(const std::string& prefix);

    // ── Mutation ─────────────────────────────
    void remove_word(const std::string& word);

    // ── Stats ────────────────────────────────
    int word_count() const;

private:
    Trie          trie_;
    RecencyCache  cache_;
    int           top_n_;
};