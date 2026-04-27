#pragma once
#include "Trie.h"
#include "Recency_cache.h"
#include <string>
#include <vector>
#include "db/IDBConnection.h"
#include "db/DictionaryRepository.h"

class AutocompleteEngine {
public:
    explicit AutocompleteEngine(int top_n = 5,
                                int cache_size = 20,
                                int recency_bonus = 100);

    void load_word(const std::string& word, int frequency = 1, int id = 0);
    void load_words(const std::vector<std::string>& words);

    bool load_from_file(const std::string& filepath);
    bool load_from_db(DictionaryRepository& dictRepo, const std::string& tableName);

    void clear();

    std::vector<Completion> query(const std::string& prefix);
    std::vector<Completion> fuzzy_query(const std::string& prefix);

    void remove_word(const std::string& word);
    int word_count() const;

private:
    Trie trie_;
    RecencyCache cache_;
    int top_n_;
};