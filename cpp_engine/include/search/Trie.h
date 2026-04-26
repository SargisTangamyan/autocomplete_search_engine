#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// Result of a prefix 
struct Completion {
    std::string word;
    int score;           // cumulative frequency + recency bonus
    int id;              // database row id (0 when loaded from file)
};

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool is_end = false;
    int  frequency = 0;
    int  id = 0;              // DB row id of this word

    ~TrieNode() {
        for (auto& [ch, child] : children)
            delete child;
    }
};

class Trie {
public:
    Trie();
    ~Trie();

    // Core operations  –  O(N) each, N = word length
    void insert(const std::string& word, int id = 0);
    int  search(const std::string& word) const;   // returns frequency, 0 if absent
    bool remove(const std::string& word);

    // Prefix query  –  O(P + S·log k), P = prefix len, S = subtree nodes, k = top_n
    // Returns up to top_n Completion entries sorted descending by score
    std::vector<Completion>
    get_completions(const std::string& prefix, int top_n = 5) const;

    int word_count() const { return word_count_; }

private:
    TrieNode* root_;
    int       word_count_;

    TrieNode* find_node(const std::string& prefix) const;
    bool      remove_helper(TrieNode* node, const std::string& word, int depth);
    void      dfs_collect(TrieNode* node,
                          std::string& current,
                          std::vector<Completion>& results) const;
};