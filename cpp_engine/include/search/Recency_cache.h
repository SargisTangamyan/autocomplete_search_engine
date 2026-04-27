#pragma once
#include "Trie.h"
#include <string>
#include <unordered_map>
#include <list>
#include <vector>
#include <algorithm>


//  RecencyCache
//  Fixed-capacity LRU cache.
//  Every recorded query gets a recency_bonus that decays
//  as newer queries push it toward the back of the LRU list.
class RecencyCache {
public:
    explicit RecencyCache(int capacity = 20, int bonus = 100)
        : capacity_(capacity), bonus_(bonus) {}

    // Record that the user searched for this term
    void record(const std::string& term) {
        auto it = map_.find(term);
        if (it != map_.end()) {
            lru_.erase(it->second.list_it);
            map_.erase(it);
        }

        lru_.push_front(term);
        map_[term] = { lru_.begin(), 1 };

        if (static_cast<int>(lru_.size()) > capacity_) {
            map_.erase(lru_.back());
            lru_.pop_back();
        }
    }

    // Returns a score boost for a term (0 if not recently searched)
    // The most recently searched term gets full bonus_;
    // older entries get a linearly decayed fraction.
    int boost_for(const std::string& term) const {
        auto it = map_.find(term);
        if (it == map_.end()) return 0;

        // Find rank in LRU list (0 = most recent)
        int rank = 0;
        for (auto lit = lru_.begin(); lit != lru_.end(); ++lit, ++rank) {
            if (*lit == term) break;
        }
        
        int total = static_cast<int>(lru_.size());
        // Linear decay: most recent → bonus_, oldest → bonus_/total
        return bonus_ * (total - rank) / total;
    }

    // Apply recency boosts to a list of Completion entries (in-place)
    void apply_boosts(std::vector<Completion>& completions) const {
        for (auto& c : completions)
            c.score += boost_for(c.word);
        std::sort(completions.begin(), completions.end(),
            [](const Completion& a, const Completion& b){ return a.score > b.score; });
    }

    bool contains(const std::string& term) const { return map_.count(term) > 0; }
    int  size()                              const { return static_cast<int>(lru_.size()); }
    void clear() { lru_.clear(); map_.clear(); }

private:
    struct Entry {
        std::list<std::string>::iterator list_it;
        int count;
    };

    int  capacity_;
    int  bonus_;
    std::list<std::string>              lru_;
    std::unordered_map<std::string, Entry> map_;
};