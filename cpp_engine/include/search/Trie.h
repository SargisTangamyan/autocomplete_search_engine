#define TRIE_H

#include <string>
#include <unordered_map>
#include <vector>

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool is_end = false;

    TrieNode() : is_end(false) {}
};

class Trie {
public:
    Trie();
    ~Trie();

    //Inserting a word into the trie
    void insert(const std::string& word);

    //Returning all words that start with the given prefix
    std::vector<std::string> search(const std::string& prefix);

    //Remove a specific word from the trie
    bool remove(const std::string& word);

private:
    TrieNode* root;

    void collectWords(TrieNode* node, const std::string& current, std::vector<std::string>& results);
    bool removeHelper(TrieNode* node, const std::string& word, int depth, bool& found);
    void destroyNode(TrieNode* node);

};