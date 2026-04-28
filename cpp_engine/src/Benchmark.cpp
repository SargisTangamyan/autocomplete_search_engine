#include "search/Autocomplete.h"
#include "search/Trie.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <random>

// ─────────────────────────────────────────────
//  Linear scan baseline
// ─────────────────────────────────────────────
std::vector<std::string> linear_scan(const std::vector<std::string>& dataset,
                                      const std::string& prefix,
                                      int top_n = 5) {
    std::vector<std::string> matches;
    for (const auto& word : dataset) {
        if (word.size() >= prefix.size() &&
            word.substr(0, prefix.size()) == prefix) {
            matches.push_back(word);
            if (static_cast<int>(matches.size()) >= top_n) break;
        }
    }
    return matches;
}

// ─────────────────────────────────────────────
//  Timing helper (returns microseconds)
// ─────────────────────────────────────────────
template<typename Fn>
long long time_us(Fn&& fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

// ─────────────────────────────────────────────
//  Benchmark runner
// ─────────────────────────────────────────────
void run_benchmark(const std::vector<std::string>& full_dataset,
                   const std::string& csv_output = "benchmark_results.csv") {

    std::vector<int> sizes = { 100, 500, 1000, 2000, 5000,
                               static_cast<int>(full_dataset.size()) };

    // Choose a handful of representative prefixes from the dataset
    std::vector<std::string> prefixes;
    std::mt19937 rng(42);
    for (int i = 0; i < 10; ++i) {
        int idx = rng() % full_dataset.size();
        const std::string& w = full_dataset[idx];
        int len = 1 + rng() % std::min((int)w.size(), 3);
        prefixes.push_back(w.substr(0, len));
    }

    std::ofstream csv(csv_output);
    csv << "dataset_size,prefix,trie_us,linear_us\n";

    for (int sz : sizes) {
        if (sz > static_cast<int>(full_dataset.size())) break;

        std::vector<std::string> subset(full_dataset.begin(),
                                        full_dataset.begin() + sz);

        // Build trie
        Trie trie;
        for (const auto& w : subset) trie.insert(w);

        for (const auto& prefix : prefixes) {
            // Warm-up
            trie.get_completions(prefix);
            linear_scan(subset, prefix);

            // Trie timing (average of 50 runs)
            long long trie_total = 0;
            for (int r = 0; r < 50; ++r)
                trie_total += time_us([&]{ trie.get_completions(prefix); });

            // Linear timing (average of 50 runs)
            long long lin_total = 0;
            for (int r = 0; r < 50; ++r)
                lin_total += time_us([&]{ linear_scan(subset, prefix); });

            long long trie_avg = trie_total / 50;
            long long lin_avg  = lin_total  / 50;

            std::cout << "size=" << sz
                      << "  prefix=\"" << prefix << "\""
                      << "  trie=" << trie_avg << "µs"
                      << "  linear=" << lin_avg << "µs\n";

            csv << sz << "," << prefix << "," << trie_avg << "," << lin_avg << "\n";
        }
    }

    csv.close();
    std::cout << "\nResults written to " << csv_output << "\n";

    // ── Inline ASCII chart ──────────────────────────────────────────────────
    std::cout << "\n=== Summary (averaged across all prefixes) ===\n";
    std::cout << std::string(55, '-') << "\n";
    std::cout << "Dataset size   │ Trie (µs) │ Linear (µs) │ Speedup\n";
    std::cout << std::string(55, '-') << "\n";

    // Re-run for the summary table (simplified)
    for (int sz : sizes) {
        if (sz > static_cast<int>(full_dataset.size())) break;
        std::vector<std::string> subset(full_dataset.begin(),
                                        full_dataset.begin() + sz);
        Trie trie;
        for (const auto& w : subset) trie.insert(w);

        long long t_sum = 0, l_sum = 0;
        for (const auto& prefix : prefixes) {
            for (int r = 0; r < 20; ++r) {
                t_sum += time_us([&]{ trie.get_completions(prefix); });
                l_sum += time_us([&]{ linear_scan(subset, prefix); });
            }
        }
        long long t_avg = t_sum / (prefixes.size() * 20);
        long long l_avg = l_sum / (prefixes.size() * 20);
        double speedup  = l_avg > 0 ? static_cast<double>(l_avg) / t_avg : 1.0;

        printf("%-14d │ %-9lld │ %-11lld │ %.1fx\n", sz, t_avg, l_avg, speedup);
    }
    std::cout << std::string(55, '-') << "\n";
}

// ─────────────────────────────────────────────
//  main  (standalone benchmark binary)
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string datafile = (argc > 1) ? argv[1] : "data/dataset.txt";

    std::vector<std::string> dataset;
    std::ifstream file(datafile);
    if (!file.is_open()) {
        std::cerr << "Cannot open dataset file: " << datafile << "\n";
        return 1;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) dataset.push_back(line);
    }

    if (dataset.size() < 100) {
        std::cerr << "Dataset too small (need ≥ 100 words).\n";
        return 1;
    }

    std::cout << "Loaded " << dataset.size() << " words. Running benchmark...\n\n";
    run_benchmark(dataset);
    return 0;
}