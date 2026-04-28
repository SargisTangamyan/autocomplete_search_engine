# Autocomplete Search Engine

A high-performance prefix autocomplete engine built in C++17 using a Trie data structure. Designed as a standalone CLI binary that integrates directly with a Laravel/Vue.js full-stack application. Laravel invokes the binary, captures the JSON output, and returns results to the frontend.

---

## Table of Contents

1. [How It Works](#how-it-works)
2. [Architecture](#architecture)
3. [Data Structure: Trie](#data-structure-trie)
4. [Features](#features)
5. [Complexity Analysis](#complexity-analysis)
6. [Project Structure](#project-structure)
7. [Prerequisites](#prerequisites)
8. [Setup](#setup)
9. [Build](#build)
10. [Running the Engine](#running-the-engine)
11. [JSON Output Format](#json-output-format)
12. [Laravel Integration](#laravel-integration)
13. [Vue.js Integration](#vuejs-integration)
14. [Database Schema](#database-schema)
15. [Fallback Dataset](#fallback-dataset)
16. [Benchmark](#benchmark)

---

## How It Works

The engine loads all words (dishes, menu items, etc.) from a MySQL database into a Trie in memory at startup. Every query traverses only the relevant prefix subtree — not the entire dataset — making completions extremely fast regardless of how large the dataset grows.

```
User types "chick" in search bar
        │
        ▼
   Vue.js frontend
        │  HTTP request  GET /api/autocomplete?q=chick
        ▼
   Laravel controller
        │  runs:  ./engine "chick"
        ▼
   C++ engine binary
        │  traverses Trie from root → c → h → i → c → k
        │  collects all words in the subtree
        │  applies recency boost from LRU cache
        │  returns top-5 by frequency, sorted
        ▼
   JSON printed to stdout
        │
        ▼
   Laravel reads stdout, returns JSON to Vue.js
        │
        ▼
   Vue.js displays suggestions
```

---

## Architecture

```
search_engine_project/
├── cpp_engine/          ← C++ engine (this project)
├── mysql/               ← Database initialization
├── docker-compose.yml   ← MySQL 8.0 service
└── .env                 ← Shared credentials
```

### Component Overview

| Component | File | Responsibility |
|---|---|---|
| `Trie` | `src/Trie.cpp` | Core data structure: insert, search, delete, prefix query, fuzzy query |
| `AutocompleteEngine` | `src/Autocomplete.cpp` | Orchestrates Trie + RecencyCache, handles normalization, loading |
| `RecencyCache` | `include/search/Recency_cache.h` | LRU cache that boosts recently searched terms |
| `MySQLConnection` | `include/db/MySQLConnection.h` | Singleton MySQL connection via MySQL Connector/C++ |
| `DictionaryRepository` | `include/db/DictionaryRepository.h` | SQL queries: insert, remove, update, getAll |
| `main.cpp` | `src/main.cpp` | CLI entry point: single-shot and REPL modes, JSON output |
| `Benchmark.cpp` | `src/Benchmark.cpp` | Standalone benchmarking tool: Trie vs linear scan |
| `Env.cpp` | `src/config/Env.cpp` | Parses `.env` key=value file into a map |

---

## Data Structure: Trie

A Trie (prefix tree) is a tree where each node represents one character. A path from root to a leaf spells out a word. Shared prefixes share nodes, which makes prefix lookup dramatically faster than scanning a list.

### Node Structure

```
struct TrieNode {
    unordered_map<char, TrieNode*> children   ← next characters
    bool  is_end    = false                   ← marks a complete word
    int   frequency = 0                       ← how many times inserted/queried
    int   id        = 0
}
```

### Example: inserting "pizza" and "pita"

```
root
 └── p
      └── i
           ├── z
           │    └── z
           │         └── a  [is_end=true, freq=5]
           └── t
                └── a  [is_end=true, freq=3]
```

Both words share the prefix `pi` — only one set of `p` and `i` nodes exists.

### Frequency Tracking

Every time a word is inserted, its terminal node's `frequency` counter increments. When a word is queried via `search()`, its frequency increments again. This means popular or frequently-searched dishes naturally rise to the top of completions over time.

---

## Features

### Insert — O(N)

Walks the Trie character by character, creating new nodes as needed. Increments `frequency` on the terminal node.

### Search — O(N)

Traverses the Trie following each character. Returns the node's frequency if `is_end == true`, 0 otherwise. Also increments frequency (counts the query itself as usage).

### Delete — O(N)

Recursive descent to the terminal node. Marks `is_end = false`, then prunes empty branches bottom-up — only removes nodes that have no other children, preserving shared prefixes.

### Top-N Prefix Completions — O(P + S·log k)

1. Find the node for the last character of the prefix — O(P)
2. DFS the entire subtree collecting all `(word, frequency)` pairs — O(S)
3. `partial_sort` to get the top-k results — O(S·log k)
4. Returns up to k entries sorted by frequency descending

Edge cases handled:
- Empty prefix → collects from the root (returns most frequent words overall)
- Prefix not in Trie → returns empty array
- Prefix subtree has fewer than k words → returns what exists

### Fuzzy Prefix Matching — edit distance ≤ 1

Allows one typo in the prefix. Three edit operations are explored at each step:

| Operation | Example | Description |
|---|---|---|
| Exact | "chick" → "chicken" | No edit spent |
| Substitution | "checken" → "chicken" | Replace one wrong character |
| Deletion | "chiken" → "chicken" | Skip one character in the prefix |
| Insertion | "cchicken" → "chicken" | Skip one character in the Trie |

Results from different edit paths are deduplicated and ranked by frequency.

### Recently Searched — LRU Cache with Decay Bonus

`RecencyCache` is a fixed-capacity (default 50) LRU cache backed by a `list<string>` (ordering) and `unordered_map` (O(1) lookup).

Every time a prefix is queried, it is recorded in the cache. When building completions, each result gets a recency bonus:

```
bonus = recency_bonus × (total_cached - rank) / total_cached
```

The most recently searched term gets the full bonus (200 by default). Older entries get a linearly decayed fraction. This means terms the user has searched for recently float to the top even if their base frequency is lower.

### Benchmark Tool

Compares Trie prefix search against a naive linear scan across dataset sizes of 100, 500, 1000, 2000, 5000, and the full dataset. Outputs:
- Per-run timing to the terminal (µs precision)
- A `benchmark_results.csv` file for plotting
- An ASCII summary table with speedup factor

---

## Complexity Analysis

| Operation | Time | Space | Notes |
|---|---|---|---|
| `insert(word)` | O(N) | O(N) | N = word length. Creates at most N new nodes |
| `search(word)` | O(N) | O(1) | Traverses exactly N nodes |
| `delete(word)` | O(N) | O(1) | Traversal + bottom-up pruning |
| `prefix_query(prefix, k)` | O(P + S·log k) | O(S) | P = prefix len, S = subtree size, k = top-N |
| `fuzzy_query(prefix, k)` | O(\|Σ\|^(P+1) · S) | O(S) | Bounded by alphabet size \|Σ\|=26 and edit budget |

**Why Trie beats linear scan:**

With a list of M words of average length L, a linear prefix scan costs O(M·L). For a Trie with the same data:
- Average subtree sizes are much smaller than M
- Shared prefixes mean fewer comparisons per node
- In benchmarks on this dataset: **3–6× faster** than linear scan at 1000 words, gap widens with dataset growth

---

## Project Structure

```
cpp_engine/
├── include/
│   ├── search/
│   │   ├── Trie.h                  ← Trie class declaration + complexity docs
│   │   ├── Autocomplete.h          ← AutocompleteEngine interface
│   │   └── Recency_cache.h         ← LRU recency cache (header-only)
│   ├── db/
│   │   ├── IDBConnection.h         ← Abstract connection interface
│   │   ├── MySQLConnection.h       ← Singleton MySQL connection
│   │   ├── IDictionaryRepository.h ← Abstract repository interface
│   │   ├── DictionaryRepository.h  ← MySQL CRUD implementation
│   │   ├── DictionaryEntry.h       ← Data model: {id, word, frequency}
│   │   ├── IDBExecutor.h           ← Abstract executor interface
│   │   └── MySQLExecutor.h         ← MySQL executor
│   └── config/
│       └── Env.h                   ← loadEnv() declaration
├── src/
│   ├── Trie.cpp                    ← All Trie operations
│   ├── Autocomplete.cpp            ← Engine: loading, querying, clearing
│   ├── Benchmark.cpp               ← Standalone benchmark binary
│   ├── main.cpp                    ← CLI entry point (single-shot + REPL)
│   └── config/
│       └── Env.cpp                 ← .env file parser
├── data/
│   └── dataset.txt                 ← 1000 food/restaurant phrases (fallback)
├── build/                          ← CMake output directory
├── tests/                          ← (reserved for unit tests)
└── CMakeLists.txt                  ← Build configuration
```

---

## Prerequisites

| Dependency | Purpose | Install |
|---|---|---|
| CMake ≥ 3.16 | Build system | `sudo apt install cmake` |
| GCC / Clang (C++17) | Compiler | `sudo apt install g++` |
| MySQL Connector/C++ | DB client library | `sudo apt install libmysqlcppconn-dev` |
| Docker + Docker Compose | Run MySQL | [docker.com](https://docker.com) |

---

## Setup

### 1. Configure credentials

The `.env` file at the project root controls all connections:

```env
MYSQL_HOST=127.0.0.1
MYSQL_ROOT=root
MYSQL_ROOT_PASSWORD=your_password
MYSQL_DATABASE=restaurant_project
MYSQL_PORT=3306
CONTAINER_NAME=mysql-container
```

The engine reads this file on startup. For production deployments where the binary is called from a different working directory, set the environment variable:

```bash
export SEARCH_ENGINE_ENV=/absolute/path/to/.env
```

### 2. Start the database

```bash
# From the project root
docker-compose up -d
```

This starts MySQL 8.0, creates the `restaurant_project` database, and runs `mysql/init.sql` which creates the `dishes` table and seeds it with 300+ dishes with realistic frequencies.

Wait a few seconds for MySQL to initialize, then verify:

```bash
docker exec -it mysql-container mysql -uroot -proot_password\
  -e "SELECT COUNT(*) FROM restaurant_project.dishes;"
```

---

## Build

```bash
cd cpp_engine

# Configure (first time only)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build --parallel
```

This produces two binaries in `cpp_engine/build/`:

| Binary | Purpose |
|---|---|
| `engine` | The autocomplete engine used by Laravel |
| `benchmark` | Standalone performance comparison tool |

---

## Running the Engine

The engine has two operating modes. Both modes print only JSON to `stdout`. All diagnostic messages go to `stderr` so they never interfere with JSON parsing.

### Single-Shot Mode

Run once per query. Laravel calls this with `shell_exec()`.

```bash
# From cpp_engine/build/
./engine "chicken"
./engine --fuzzy "chicen"         # fuzzy: edit distance ≤ 1
./engine --top 10 "pasta"        # return up to 10 results (default 5)
```

Output is one JSON line, then the process exits.

### REPL / Pipe Mode

The process starts once and stays alive. Send queries line by line via stdin, receive one JSON line per response. Laravel uses `proc_open()` for this. Avoids the DB connection overhead on every request.

```bash
./engine
# then type queries interactively or pipe them:

echo "pasta" | ./engine
printf "pizza\nchicken\npasta\n" | ./engine
```

Special commands in REPL mode:

| Input | Action | Output |
|---|---|---|
| `chicken` | Normal prefix query | JSON results |
| `?chicen` | Fuzzy query (edit distance ≤ 1) | JSON results |
| `+chicken tikka` | Insert word into Trie | `{"status":"inserted","word":"chicken tikka"}` |
| `-chicken tikka` | Remove word from Trie | `{"status":"removed","word":"chicken tikka"}` |
| `:stats` | Word count | `{"words":1000}` |
| `:quit` | Exit the process | — |

### Environment Override

```bash
SEARCH_ENGINE_ENV=/var/www/html/.env ./engine "pasta"
```

### Fallback Behavior

On startup the engine:
1. Reads `.env` and connects to MySQL
2. Loads all rows from the `dishes` table (word + frequency)
3. If the DB is unavailable **or** the table is empty, loads `data/dataset.txt` instead (1000 food/restaurant phrases)
4. Starts accepting queries

---

## JSON Output Format

Every query response — whether single-shot or REPL — is a single JSON object followed by a newline.

```json
{
  "query":      "chick",
  "fuzzy":      false,
  "elapsed_ms": 0.027,
  "count":      5,
  "results": [
    { "word": "chicken tikka masala", "score": 298 },
    { "word": "chicken biryani",      "score": 297 },
    { "word": "chicken korma",        "score": 287 },
    { "word": "chicken katsu",        "score": 191 },
    { "word": "chicken burger",       "score": 92  }
  ]
}
```

| Field | Type | Description |
|---|---|---|
| `query` | string | The prefix that was searched |
| `fuzzy` | boolean | Whether fuzzy matching was used |
| `elapsed_ms` | float | Engine query time in milliseconds (not including DB load) |
| `count` | integer | Number of results returned |
| `results` | array | Up to N `{word, score}` objects, sorted by score descending |
| `results[].word` | string | The matched word or phrase |
| `results[].score` | integer | Composite score: base frequency + recency bonus |

When no completions are found:

```json
{"query":"xyzxyz","fuzzy":false,"elapsed_ms":0.001,"count":0,"results":[]}
```

---

## Laravel Integration

### Option A — shell_exec (simple, one process per query)

Best for low-traffic or development use. Each request spawns a new process which connects to MySQL and loads the dataset.

```php
// routes/api.php
Route::get('/autocomplete', [SearchController::class, 'autocomplete']);
```

```php
// app/Http/Controllers/SearchController.php
<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

class SearchController extends Controller
{
    private string $enginePath;
    private string $envPath;

    public function __construct()
    {
        $this->enginePath = base_path('../search_engine_project/cpp_engine/build/engine');
        $this->envPath    = base_path('../search_engine_project/.env');
    }

    public function autocomplete(Request $request): JsonResponse
    {
        $query = trim($request->input('q', ''));

        if ($query === '') {
            return response()->json(['query' => '', 'count' => 0, 'results' => []]);
        }

        $fuzzy  = $request->boolean('fuzzy') ? '--fuzzy ' : '';
        $top    = (int) $request->input('top', 5);

        $cmd = sprintf(
            'SEARCH_ENGINE_ENV=%s %s %s--top %d %s 2>/dev/null',
            escapeshellarg($this->envPath),
            escapeshellarg($this->enginePath),
            $fuzzy,
            $top,
            escapeshellarg($query)
        );

        $output = shell_exec($cmd);

        if (!$output) {
            return response()->json(['error' => 'Engine unavailable'], 503);
        }

        return response()->json(json_decode($output, true));
    }
}
```

### Option B — proc_open (persistent process, recommended for production)

The engine process starts once and stays alive for the lifetime of the PHP worker. Each query sends one line to stdin and reads one line from stdout. This avoids the MySQL connection overhead on every request, making it suitable for autocomplete on every keystroke.

```php
// app/Services/SearchEngineService.php
<?php

namespace App\Services;

class SearchEngineService
{
    private static mixed $process = null;
    private static array $pipes   = [];

    private static function ensureRunning(): void
    {
        if (self::$process && proc_get_status(self::$process)['running']) {
            return;
        }

        $enginePath = base_path('../search_engine_project/cpp_engine/build/engine');
        $envPath    = base_path('../search_engine_project/.env');

        $descriptors = [
            0 => ['pipe', 'r'],   // stdin  → engine
            1 => ['pipe', 'w'],   // stdout ← engine
            2 => ['file', '/dev/null', 'w'],
        ];

        self::$process = proc_open(
            sprintf('SEARCH_ENGINE_ENV=%s %s',
                escapeshellarg($envPath),
                escapeshellarg($enginePath)
            ),
            $descriptors,
            self::$pipes
        );

        // Non-blocking stdout so fgets does not hang on empty results
        stream_set_blocking(self::$pipes[1], false);
    }

    public static function query(string $prefix, bool $fuzzy = false): array
    {
        self::ensureRunning();

        $line = ($fuzzy ? '?' : '') . $prefix . "\n";
        fwrite(self::$pipes[0], $line);
        fflush(self::$pipes[0]);

        // Give the engine up to 50 ms to respond
        $deadline = microtime(true) + 0.05;
        $response = '';
        while (microtime(true) < $deadline) {
            $response = fgets(self::$pipes[1]);
            if ($response !== false) break;
            usleep(1000);
        }

        return $response ? (json_decode($response, true) ?? []) : [];
    }

    public static function shutdown(): void
    {
        if (self::$process) {
            fwrite(self::$pipes[0], ":quit\n");
            proc_close(self::$process);
            self::$process = null;
        }
    }
}
```

```php
// Controller using the service
public function autocomplete(Request $request): JsonResponse
{
    $query = trim($request->input('q', ''));
    if ($query === '') {
        return response()->json(['count' => 0, 'results' => []]);
    }

    $data = SearchEngineService::query($query, $request->boolean('fuzzy'));
    return response()->json($data);
}
```

Register the service in `AppServiceProvider` or use Laravel's service container as needed.

---

## Vue.js Integration

Debounce the search so the engine is not called on every single keystroke. 300 ms is a comfortable threshold for autocomplete.

```vue
<template>
  <div class="search-wrapper">
    <input
      v-model="query"
      type="text"
      placeholder="Search dishes..."
      @input="onInput"
      autocomplete="off"
    />
    <ul v-if="suggestions.length" class="suggestions">
      <li
        v-for="item in suggestions"
        :key="item.word"
        @click="select(item.word)"
      >
        {{ item.word }}
      </li>
    </ul>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import axios from 'axios'

const query       = ref('')
const suggestions = ref([])
let debounceTimer = null

function onInput() {
  clearTimeout(debounceTimer)
  if (!query.value.trim()) {
    suggestions.value = []
    return
  }
  debounceTimer = setTimeout(async () => {
    try {
      const { data } = await axios.get('/api/autocomplete', {
        params: { q: query.value }
      })
      suggestions.value = data.results ?? []
    } catch {
      suggestions.value = []
    }
  }, 300)
}

function select(word) {
  query.value = word
  suggestions.value = []
}
</script>
```

---

## Database Schema

```sql
CREATE TABLE IF NOT EXISTS dishes (
    id        INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name      VARCHAR(255) NOT NULL,
    frequency INT          NOT NULL DEFAULT 1
);
```

| Column | Type | Description |
|---|---|---|
| `id` | INT AUTO_INCREMENT | Primary key |
| `name` | VARCHAR(255) | The dish name or phrase loaded into the Trie |
| `frequency` | INT | Initial weight. Higher values mean the word ranks higher from the first query |

The engine loads all rows with `SELECT id, name, frequency FROM dishes` on startup. Each row is inserted into the Trie `frequency` times, so a dish with `frequency=95` starts with a higher rank than one with `frequency=1`.

When Laravel adds a new dish via its own CRUD operations, the engine picks it up on the next restart. In the REPL mode you can also insert live:

```bash
echo "+grilled halloumi" | ./engine
```

---

## Fallback Dataset

`data/dataset.txt` contains 1000 food and restaurant phrases — one per line — covering Italian, French, American, Japanese, Chinese, Indian, Mexican, Thai, Mediterranean, Greek, Spanish, German, Korean, Vietnamese, Middle Eastern cuisines, plus desserts, beverages, and cocktails.

The engine uses it automatically when:
- The MySQL container is not running
- The `dishes` table is empty

You can also use it directly to seed the benchmark:

```bash
./benchmark ../data/dataset.txt
```

---

## Benchmark

Run the dedicated benchmark binary to compare Trie prefix search against a linear scan on the same dataset:

```bash
cd cpp_engine/build
./benchmark ../data/dataset.txt
```

The benchmark:
1. Loads the full dataset from the file
2. Selects 10 random representative prefixes (lengths 1–3)
3. For each dataset size (100, 500, 1000, 2000, 5000, full):
   - Builds a fresh Trie
   - Runs 50 timed iterations per prefix for both Trie and linear scan
   - Prints per-run timings (µs)
4. Prints a summary table with average times and speedup factor
5. Writes all raw numbers to `benchmark_results.csv` for plotting

Example output:

```
Dataset size   │ Trie (µs) │ Linear (µs) │ Speedup
-------------------------------------------------------
100            │ 1         │ 4           │ 4.0x
500            │ 2         │ 12          │ 6.0x
1000           │ 6         │ 23          │ 3.8x
-------------------------------------------------------
```

The Trie advantage grows with dataset size because the subtree it needs to scan stays proportional to the number of words matching the prefix — not the total number of words.

---

## Quick Reference

```bash
# Start database
docker-compose up -d

# Build
cd cpp_engine && cmake -S . -B build && cmake --build build --parallel

# Single query
./build/engine "pasta"

# Fuzzy query (1 typo allowed)
./build/engine --fuzzy "pastta"

# REPL mode (pipe-friendly)
echo -e "pizza\n?chicen\n:stats\n:quit" | ./build/engine

# Benchmark
./build/benchmark data/dataset.txt
```
