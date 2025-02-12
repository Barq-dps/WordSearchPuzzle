#define CPPHTTPLIB_OPENSSL_SUPPORT
#define NOMINMAX
#include "WordSearchSolver.h"
#include <iostream>
#include <random>
#include <thread>
#include <cctype>
#include <unordered_set>
#include "httplib.h"
#include <mutex>
#include <atomic>
#include <algorithm>
#include <future>
#include "json.hpp" 
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
using json = nlohmann::json;

// **Global Target Words List**
std::vector<std::string> targetWords;
std::mutex apiMutex;  // Mutex for API synchronization

WordSearchSolver::WordSearchSolver() {
    grid.clear();
}

// **API Response Handling**
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}


// Capitalize a string
static std::string capitalizeWord(const std::string& word) {
    std::string capitalized = word;
    for (char& c : capitalized) {
        c = std::toupper(c);
    }
    return capitalized;
}

// Validate a word using the Dictionary API via httplib
static bool isValidEnglishWord(const std::string& word) {
    std::lock_guard<std::mutex> lock(apiMutex);

    std::string host = "api.dictionaryapi.dev";
    std::string path = "/api/v2/entries/en/" + word;
    httplib::SSLClient cli(host.c_str());
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);

    if (auto res = cli.Get(path.c_str())) {
        if (res->status == 200 && res->body.find("\"title\":\"No Definitions Found\"") == std::string::npos) {
            return true;
        }
    }
    return false;
}

static std::vector<std::string> fetchValidWords(int wordCount, int minLen, int maxLen) {
    std::vector<std::string> validWords;
    int batchSize = std::min(30, wordCount);
    int wordsFetched = 0;

    while (wordsFetched < wordCount) {
        int wordLength = minLen + (rand() % (maxLen - minLen + 1));
        std::string host = "random-word-api.herokuapp.com";
        std::string path = "/word?length=" + std::to_string(wordLength) + "&number=" + std::to_string(batchSize);
        httplib::SSLClient cli(host.c_str());
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(3, 0);

        auto res = cli.Get(path.c_str());
        if (!res || res->status != 200) {
            continue;
        }

        try {
            // Parse the response into a JSON array.
            auto j = json::parse(res->body);
            // Expecting an array of words, e.g., ["word1", "word2", ...]
            for (auto& element : j) {
                std::string word = element.get<std::string>();
                word = capitalizeWord(word);
                auto futureValidation = std::async(std::launch::async, isValidEnglishWord, word);
                if (futureValidation.get()) {
                    validWords.push_back(word);
                    wordsFetched++;
                    if (wordsFetched >= wordCount) break;
                }
            }
        }
        catch (json::parse_error& e) {
            std::cerr << "JSON Parse Error: " << e.what() << std::endl;
            continue;
        }
    }

    return validWords;
}

bool WordSearchSolver::placeWordInGrid(const std::string& word) {
    int size = grid.size();
    const int maxAttempts = 100;  // You can adjust this as needed
    int attempts = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<std::pair<int, int>> directions = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    while (attempts < maxAttempts) {
        // Randomize starting position and directions
        int x = rand() % size;
        int y = rand() % size;
        std::shuffle(directions.begin(), directions.end(), gen);

        for (const auto& [dx, dy] : directions) {
            int nx = x, ny = y, i;
            for (i = 0; i < word.length(); ++i) {
                if (nx < 0 || ny < 0 || nx >= size || ny >= size ||
                    (grid[nx][ny] != ' ' && grid[nx][ny] != word[i]))
                {
                    break;
                }
                nx += dx;
                ny += dy;
            }
            if (i == word.length()) {  // Valid placement found
                nx = x;
                ny = y;
                for (char c : word) {
                    grid[nx][ny] = c;
                    nx += dx;
                    ny += dy;
                }
                return true;  // Indicate that the word was placed
            }
        }
        attempts++;
    }
    return false;  // Failed to place the word after maxAttempts
}


void WordSearchSolver::loadGrid(int size) {
    grid.resize(size, std::vector<char>(size, ' '));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis('A', 'Z');

    int numWords = (size + size) / 2;  // (Rows + Columns) / 2
    int minWordLength = std::max(3, size / 4);
    int maxWordLength = size;

    // Fetch words asynchronously
    std::future<std::vector<std::string>> futureWords =
        std::async(std::launch::async, fetchValidWords, numWords, minWordLength, maxWordLength);
    std::vector<std::string> fetchedWords = futureWords.get();

    // Try to place each word; only keep the ones that are successfully placed
    targetWords.clear();
    for (const std::string& word : fetchedWords) {
        if (placeWordInGrid(word)) {
            targetWords.push_back(word);
        }
    }

    // Fill only the empty spaces with random letters
    for (auto& row : grid) {
        for (auto& cell : row) {
            if (cell == ' ') {
                cell = dis(gen);
            }
        }
    }
}
void WordSearchSolver::saveGridToFile(const std::string& filename) {
    // Create an output directory if it doesn't exist.
    fs::path outputDir = fs::current_path() / "output";
    if (!fs::exists(outputDir)) {
        fs::create_directory(outputDir);
    }
    fs::path fullPath = outputDir / filename;

    std::ofstream file(fullPath);
    if (!file) {
        std::cerr << "Error opening file: " << fullPath << std::endl;
        return;
    }

    file << "Word Search Grid:\n";
    for (const auto& row : grid) {
        for (char c : row) {
            file << c << " ";
        }
        file << "\n";
    }

    file << "\nFind these words:\n";
    for (const std::string& word : targetWords) {
        file << word << "\n";
    }
    file.close();
    std::cout << "Grid saved to " << fullPath << std::endl;
}



// **Display Grid and Words**
void WordSearchSolver::displayGrid() {
    std::cout << "\nWord Search Grid:\n";
    for (const auto& row : grid) {
        for (char c : row) {
            std::cout << c << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\nFind these words:\n";
    for (const std::string& word : targetWords) {
        std::cout << word << std::endl;
    }
}

// **Solve (Placeholder)**
std::vector<std::string> WordSearchSolver::solve() {
    return targetWords;
}