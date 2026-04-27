#include "search/Autocomplete.h"
#include "search/Trie.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

static std::string normalize(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

AutocompleteEngine::AutocompleteEngine(int top_n, int cache_size, int recency_bonus)
    : cache_(cache_size, recency_bonus), top_n_(top_n) {}

void AutocompleteEngine::load_word(const std::string& word, int frequency, int id) {
    std::string norm = normalize(word);
    if (norm.empty()) return;

    for (int i = 0; i < frequency; ++i) {
        trie_.insert(norm, id);
    }
}

void AutocompleteEngine::load_words(const std::vector<std::string>& words) {
    for (const auto& w : words) {
        load_word(w);
    }
}

bool AutocompleteEngine::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[AutocompleteEngine] Cannot open file: " << filepath << "\n";
        return false;
    }

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (!line.empty()) {
            load_word(line);
            ++count;
        }
    }

    std::cerr << "[engine] Loaded " << count << " words from file '" << filepath << "'\n";
    return true;
}

bool AutocompleteEngine::load_from_db(DictionaryRepository& dictRepo, const std::string& tableName) {
    auto entries = dictRepo.getAll(tableName);
    int count = 0;

    for (auto& entry : entries) {
        load_word(entry.word, entry.frequency, entry.id);
        ++count;
    }

    std::cerr << "[engine] Loaded " << count << " words from table '" << tableName << "'\n";
    return true;
}

void AutocompleteEngine::clear() {
    trie_ = Trie();
    cache_.clear();
}

std::vector<Completion> AutocompleteEngine::query(const std::string& prefix) {
    std::string norm = normalize(prefix);

    if (!norm.empty())
        cache_.record(norm);

    auto results = trie_.get_completions(norm, top_n_);

    if (results.empty()) return {};

    cache_.apply_boosts(results);

    if (static_cast<int>(results.size()) > top_n_)
        results.resize(top_n_);

    return results;
}

std::vector<Completion> AutocompleteEngine::fuzzy_query(const std::string& prefix) {
    std::string norm = normalize(prefix);

    if (!norm.empty())
        cache_.record(norm);

    auto results = trie_.get_fuzzy_completions(norm, top_n_);

    if (results.empty()) return {};

    cache_.apply_boosts(results);

    if (static_cast<int>(results.size()) > top_n_)
        results.resize(top_n_);

    return results;
}

void AutocompleteEngine::remove_word(const std::string& word) {
    (void)word;
}

int AutocompleteEngine::word_count() const {
    return trie_.word_count();
}