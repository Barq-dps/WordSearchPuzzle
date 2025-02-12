#ifndef WORD_SEARCH_SOLVER_H
#define WORD_SEARCH_SOLVER_H

#include <vector>
#include <string>

class WordSearchSolver {
private:
    std::vector<std::vector<char>> grid;
    std::vector<std::string> targetWords; // Stores words to find

    // Places a word into the grid at a random valid position
    bool placeWordInGrid(const std::string& word);

    // Fetches valid words of specified lengths from the API
    //static std::vector<std::string> fetchValidWords(int wordCount, int minLen, int maxLen);

public:
    WordSearchSolver();
    void loadGrid(int size);
    void displayGrid();
    std::vector<std::string> solve();
    void saveGridToFile(const std::string& filename);
    //const std::vector<std::vector<char>>& getGrid() const;
    const std::vector<std::vector<char>>& getGrid() const {
        return grid;
    }
};

#endif