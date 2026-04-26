#include "search/Trie.h"
#include <algorithm>

//  Construction / destruction
Trie::Trie() : root_(new TrieNode()), word_count_(0) {}
Trie::~Trie() { delete root_; }

//  insert  –  O(N)
void Trie::insert(const std::string& word, int id) {
    if (word.empty()) return;
    TrieNode* cur = root_;

    for (char ch : word) {
        auto it = cur->children.find(ch);
        if (it == cur->children.end()) {
            cur->children[ch] = new TrieNode();
            it = cur->children.find(ch);
        }
        cur = it->second;
    }

    if (!cur->is_end) {
        cur->is_end = true;
        ++word_count_;
    }

    ++cur->frequency;
    cur->id = id;
}

//  search  –  O(N)
int Trie::search(const std::string& word) const {
    TrieNode* node = find_node(word);

    if (node && node->is_end) {
        ++node->frequency;
        return node->frequency;
    }

    return 0;
}

//  remove  –  O(N)
bool Trie::remove(const std::string& word) {
    if (!search(word)) return false;
    remove_helper(root_, word, 0);
    --word_count_;


    return true;
}

bool Trie::remove_helper(TrieNode* node, const std::string& word, int depth) {
    if (!node) return false;

    if (depth == static_cast<int>(word.size())) {
        if (!node->is_end) return false;
        node->is_end = false;
        node->frequency = 0;
        node->id = 0;
        return node->children.empty();
    }

    char ch = word[depth];
    auto it = node->children.find(ch);
    if (it == node->children.end()) return false;

    bool should_delete = remove_helper(it->second, word, depth + 1);
    if (should_delete) {
        delete it->second;
        node->children.erase(it);
        return !node->is_end && node->children.empty();
    }

    return false;
}

//  find_node helper  –  O(N)
TrieNode* Trie::find_node(const std::string& prefix) const {
    TrieNode* cur = root_;

    for (char ch : prefix) {
        auto it = cur->children.find(ch);
        if (it == cur->children.end()) return nullptr;
        cur = it->second;
    }

    return cur;
}

//  get_completions  –  O(P + S·log k)
std::vector<Completion>
Trie::get_completions(const std::string& prefix, int top_n) const {
    std::vector<Completion> all_results;
    TrieNode* start = prefix.empty() ? root_ : find_node(prefix);
    if (!start) return {};

    std::string current = prefix;
    dfs_collect(start, current, all_results);

    int k = std::min(top_n, static_cast<int>(all_results.size()));
    std::partial_sort(all_results.begin(), all_results.begin() + k, all_results.end(),
        [](const Completion& a, const Completion& b){ return a.score > b.score; });

    all_results.resize(k);
    return all_results;
}

//  DFS collector
void Trie::dfs_collect(TrieNode* node, std::string& current, std::vector<Completion>& results) const {
    if (node->is_end)
        results.push_back({ current, node->frequency, node->id });

    for (auto& [ch, child] : node->children) {
        current.push_back(ch);
        dfs_collect(child, current, results);
        current.pop_back();
    }
}