#include "../include/search/Trie.h"
#include <iostream>

Trie::Trie() {
    root = new TrieNode();
}

Trie::~Trie() {
    destroyNode(root);
}

void Trie:: insert (const std::string& word) {
    TrieNode* current = root;
    for (char ch : word) {
        if (current->children.find(ch) == current->children.end()) {
            current->children[ch] = new TrieNode();
        }
        current = current->children[ch];
    }
    current->is_end = true;
}

std::vector<std::string> Trie::search(const std::string& prefix) {
    TrieNode* current = root;

    for (char ch : prefix) {
        if (current->children.find(ch) == current->children.end()) {
            return {};
        }
        current = current->children[ch];
    }

    std::vector<std::string> results;
    collectWords(current, prefix, results);
    return results;
}

bool Trie::remove(const std::string& word) {
    bool found = false;
    removeHelper(root, word, 0, found);
    return found;
}

void Trie::collectWords(TrieNode* node, const std::string& current, std::vector<std::string>& results) {
    if (node->is_end) {
        results.push_back(current);
    }
    for (auto& pair : node->children) {
        collectWords(pair.second, current + pair.first, results);
    }
}

bool Trie::removeHelper(TrieNode* node, const std::string& word, int depth, bool& found) {
    if (depth == (int)word.size()) {
        if (node->is_end) {
            node->is_end = false;
            found = true;           // word was actually found and removed
        }
        return node->children.empty();
    }

    char ch = word[depth];
    if (node->children.find(ch) == node->children.end()) {
        return false;
    }

    bool shouldDelete = removeHelper(node->children[ch], word, depth + 1, found);

    if (shouldDelete) {
        delete node->children[ch];
        node->children.erase(ch);
        return node->children.empty() && !node->is_end;
    }

    return false;
}

void Trie::destroyNode(TrieNode* node) {
    for (auto& pair : node->children) {
        destroyNode(pair.second);
    }
    delete node;
}


//This is a local test
/*
int main() {
    Trie trie;

    trie.insert("apple");
    trie.insert("application");
    trie.insert("apply");
    trie.insert("banana");
    trie.insert("band");

    // Search test
    std::vector<std::string> results = trie.search("app");
    std::cout << "Search 'app':" << std::endl;
    for (const std::string& word : results) {
        std::cout << "  " << word << std::endl;
    }

    // Remove test
    bool removed = trie.remove("apple");
    std::cout << "Removed 'apple': " << (removed ? "yes" : "no") << std::endl;

    results = trie.search("app");
    std::cout << "Search 'app' after removal:" << std::endl;
    for (const std::string& word : results) {
        std::cout << "  " << word << std::endl;
    }
    

    // Remove non-existent word test
    bool removedFake = trie.remove("cat");
    std::cout << "Removed 'cat': " << (removedFake ? "yes" : "no") << std::endl;

    return 0;
}
*/
