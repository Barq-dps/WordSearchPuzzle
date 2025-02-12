#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>
#include <vector>
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
#include <random>
#include "WordSearchSolver.h"  

namespace fs = std::filesystem;
using json = nlohmann::json;

// Global UI State Variables (used by the ImGui interface)
static bool modeSelected = false;	// True when a difficulty mode is chosen
static int selectedGridSize = 0;	// 5 for Easy, 10 for Medium, 15 for Expert
static std::vector<std::string> targetWords;  // Target words from solver.solve()
static std::vector<bool> foundFlags;	// Flag for each target word (found or not)
static std::vector<ImVec4> foundWordColors;     // Color assigned for each found word
static std::string currentSelection;	        // Current selection string (from grid)
static std::vector<std::pair<int, int>> selectedCells; // Currently selected (but not finalized) cell coordinates
static std::vector<std::pair<int, int>> usedCells;     // Cells that are finalized (used in a found word)
static std::vector<ImVec4> usedCellColors;      // Colors assigned to each used cell

// Variables for asynchronous puzzle loading.
static bool loading = false;
static std::future<std::vector<std::string>> loadingFuture;

// Fixed Palette of 15 Colors for Found Words
static const std::vector<ImVec4> fixedPalette = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),    // Red
    ImVec4(0.0f, 1.0f, 0.0f, 1.0f),    // Green
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f),    // Blue
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f),    // Yellow
    ImVec4(1.0f, 0.0f, 1.0f, 1.0f),    // Magenta
    ImVec4(0.0f, 1.0f, 1.0f, 1.0f),    // Cyan
    ImVec4(1.0f, 0.5f, 0.0f, 1.0f),    // Orange
    ImVec4(0.5f, 0.0f, 0.5f, 1.0f),    // Purple
    ImVec4(0.5f, 0.5f, 0.5f, 1.0f),    // Gray
    ImVec4(0.0f, 0.5f, 0.5f, 1.0f),    // Teal
    ImVec4(0.5f, 0.0f, 0.0f, 1.0f),    // Dark Red
    ImVec4(0.0f, 0.5f, 0.0f, 1.0f),    // Dark Green
    ImVec4(0.0f, 0.0f, 0.5f, 1.0f),    // Dark Blue
    ImVec4(1.0f, 0.75f, 0.8f, 1.0f),   // Pink
    ImVec4(0.8f, 0.8f, 0.0f, 1.0f)     // Olive
};

// Helper Functions
bool isCellUsed(const std::vector<std::pair<int, int>>& cells, int i, int j) {
    for (const auto& p : cells) {
        if (p.first == i && p.second == j)
            return true;
    }
    return false;
}

// Main Function & ImGui Loop
int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Word Search Game");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML." << std::endl;
        return -1;
    }
    WordSearchSolver solver;
    sf::Clock deltaClock;
    // Flag to ensure we open the "Puzzle Completed" popup only once.
    static bool puzzleCompletedPopupShown = false;

    while (window.isOpen()) {
        // Process SFML events.
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
                window.close();
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        // Check if asynchronous grid loading is complete.
        if (loading) {
            if (loadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                targetWords = loadingFuture.get();
                foundFlags = std::vector<bool>(targetWords.size(), false);
                foundWordColors = std::vector<ImVec4>(targetWords.size(), ImVec4(1, 1, 1, 1));
                loading = false;
            }
        }

        // Create a full-screen ImGui window.
        float windowWidth = static_cast<float>(window.getSize().x);
        float windowHeight = static_cast<float>(window.getSize().y);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("##FullScreen", nullptr, windowFlags);

        // Top Bar (when a mode is selected).
        if (modeSelected) {
            if (ImGui::Button("Back to Mode Selection")) {
                modeSelected = false;
                selectedGridSize = 0;
                targetWords.clear();
                foundFlags.clear();
                foundWordColors.clear();
                currentSelection.clear();
                selectedCells.clear();
                usedCells.clear();
                usedCellColors.clear();
                solver = WordSearchSolver(); // Reinitialize the solver.
                loading = false;
                puzzleCompletedPopupShown = false;
            }
            ImGui::Separator();
        }

        // Mode Selection Screen
        if (!modeSelected) {
            ImGui::SetWindowFontScale(2.0f);
            ImGui::Text("Select a Difficulty Mode:");
            ImGui::Spacing();
            if (ImGui::Button("Easy (5x5)")) {
                selectedGridSize = 5;
                modeSelected = true;
                currentSelection.clear();
                selectedCells.clear();
                usedCells.clear();
                usedCellColors.clear();
                loading = true;
                loadingFuture = std::async(std::launch::async, [&solver]() {
                    solver.loadGrid(selectedGridSize);
                    return solver.solve();
                    });
            }
            ImGui::Spacing();
            if (ImGui::Button("Medium (10x10)")) {
                selectedGridSize = 10;
                modeSelected = true;
                currentSelection.clear();
                selectedCells.clear();
                usedCells.clear();
                usedCellColors.clear();
                loading = true;
                loadingFuture = std::async(std::launch::async, [&solver]() {
                    solver.loadGrid(selectedGridSize);
                    return solver.solve();
                    });
            }
            ImGui::Spacing();
            if (ImGui::Button("Expert (15x15)")) {
                selectedGridSize = 15;
                modeSelected = true;
                currentSelection.clear();
                selectedCells.clear();
                usedCells.clear();
                usedCellColors.clear();
                loading = true;
                loadingFuture = std::async(std::launch::async, [&solver]() {
                    solver.loadGrid(selectedGridSize);
                    return solver.solve();
                    });
            }
            ImGui::Separator();
            if (!targetWords.empty()) {
                ImGui::Text("Found Words from Last Session:");
                for (const auto& word : targetWords) {
                    ImGui::SameLine();
                    ImGui::Text("[%s] ", word.c_str());
                }
                ImGui::NewLine();
                ImGui::Separator();
            }
        }
        // Puzzle Interface
        else {
            if (loading) {
                ImGui::Text("Loading puzzle, please wait...");
            }
            else {
                // Two-column layout.
                ImGui::Columns(2, nullptr, true);

                // Left Column: Puzzle Grid.
                ImGui::Text("Puzzle Grid:");
                ImGui::Spacing();
                const auto& grid = solver.getGrid();
                float cellSize = 40.0f;
                for (int i = 0; i < selectedGridSize; ++i) {
                    for (int j = 0; j < selectedGridSize; ++j) {
                        std::string cellLabel = std::string(1, grid[i][j]) + "##" +
                            std::to_string(i) + "_" + std::to_string(j);
                        if (isCellUsed(usedCells, i, j)) {
                            int idx = -1;
                            for (int k = 0; k < usedCells.size(); k++) {
                                if (usedCells[k].first == i && usedCells[k].second == j) {
                                    idx = k;
                                    break;
                                }
                            }
                            if (idx >= 0 && idx < usedCellColors.size())
                                ImGui::PushStyleColor(ImGuiCol_Button, usedCellColors[idx]);
                            else
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
                            if (ImGui::Button(cellLabel.c_str(), ImVec2(cellSize, cellSize))) {
                                // Always allow pressing to add the character.
                                selectedCells.push_back({ i, j });
                                currentSelection.push_back(grid[i][j]);
                            }
                            ImGui::PopStyleColor();
                        }
                        else {
                            bool alreadySelected = isCellUsed(selectedCells, i, j);
                            bool canSelect = false;
                            if (!alreadySelected) {
                                if (selectedCells.empty()) {
                                    canSelect = true;
                                }
                                else if (selectedCells.size() == 1) {
                                    auto last = selectedCells.back();
                                    if (std::abs(i - last.first) <= 1 && std::abs(j - last.second) <= 1)
                                        canSelect = true;
                                }
                                else {
                                    std::pair<int, int> lockedDirection = { selectedCells[1].first - selectedCells[0].first,
                                                                            selectedCells[1].second - selectedCells[0].second };
                                    auto last = selectedCells.back();
                                    if (i == last.first + lockedDirection.first &&
                                        j == last.second + lockedDirection.second)
                                        canSelect = true;
                                }
                            }
                            if (alreadySelected) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                                if (ImGui::Button(cellLabel.c_str(), ImVec2(cellSize, cellSize))) {
                                    selectedCells.push_back({ i, j });
                                    currentSelection.push_back(grid[i][j]);
                                }
                                ImGui::PopStyleColor();
                            }
                            else if (canSelect) {
                                if (ImGui::Button(cellLabel.c_str(), ImVec2(cellSize, cellSize))) {
                                    selectedCells.push_back({ i, j });
                                    currentSelection.push_back(grid[i][j]);
                                }
                            }
                            else {
                                ImGui::BeginDisabled();
                                ImGui::Button(cellLabel.c_str(), ImVec2(cellSize, cellSize));
                                ImGui::EndDisabled();
                            }
                        }
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }
                ImGui::NextColumn();

                // Right Column: Current Selection Group.
                float groupWidth = 250.0f;
                float availWidth = ImGui::GetContentRegionAvail().x;
                float offsetX = (availWidth - groupWidth) * 0.5f;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                ImGui::BeginChild("SelectionGroup", ImVec2(groupWidth, 100), false);
                ImGui::Text("Current Selection:");
                ImGui::Separator();
                ImGui::TextWrapped("%s", currentSelection.c_str());
                ImGui::Spacing();
                if (ImGui::Button("Clear", ImVec2(100, 30))) {
                    currentSelection.clear();
                    selectedCells.clear();
                }
                ImGui::SameLine();
                if (ImGui::Button("Check Word                                                 ", ImVec2(250, 30))) {
                    bool correct = false;
                    int foundIndex = -1;
                    for (size_t k = 0; k < targetWords.size(); k++) {
                        if (targetWords[k] == currentSelection) {
                            foundIndex = static_cast<int>(k);
                            if (foundFlags[k]) {
                                ImGui::OpenPopup("Already Found");
                                correct = false;
                            }
                            else {
                                correct = true;
                            }
                            break;
                        }
                    }
                    if (correct && foundIndex >= 0) {
                        foundFlags[foundIndex] = true;
                        foundWordColors[foundIndex] = fixedPalette[foundIndex % fixedPalette.size()];
                        for (const auto& cell : selectedCells) {
                            if (!isCellUsed(usedCells, cell.first, cell.second)) {
                                usedCells.push_back(cell);
                                usedCellColors.push_back(foundWordColors[foundIndex]);
                            }
                        }
                        ImGui::OpenPopup("Correct");
                    }
                    if (!correct && foundIndex == -1) {
                        ImGui::OpenPopup("Incorrect");
                    }
                }
                if (ImGui::BeginPopupModal("Correct", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Correct! You found the word.");
                    if (ImGui::Button("OK")) {
                        ImGui::CloseCurrentPopup();
                        currentSelection.clear();
                        selectedCells.clear();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Already Found", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Word already found.");
                    if (ImGui::Button("OK")) {
                        ImGui::CloseCurrentPopup();
                        currentSelection.clear();
                        selectedCells.clear();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Incorrect", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Incorrect word. Please try again.");
                    if (ImGui::Button("OK")) {
                        ImGui::CloseCurrentPopup();
                        currentSelection.clear();
                        selectedCells.clear();
                    }
                    ImGui::EndPopup();
                }
                ImGui::EndChild();
                ImGui::Columns(1);

                // Bottom: Display Target Words as Pressable Buttons.
                ImGui::Separator();
                ImGui::Text("Target Words:");
                for (size_t k = 0; k < targetWords.size(); k++) {
                    if (k > 0 && k % 10 == 0)
                        ImGui::NewLine();
                    else if (k > 0)
                        ImGui::SameLine();
                    ImVec2 textSize = ImGui::CalcTextSize(targetWords[k].c_str());
                    float padding = 10.0f;
                    float buttonWidth = textSize.x + padding;
                    float buttonHeight = 30.0f;
                    ImVec4 btnColor = foundFlags[k] ? fixedPalette[k % fixedPalette.size()]
                        : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                    ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                    if (ImGui::Button(targetWords[k].c_str(), ImVec2(buttonWidth, buttonHeight))) {
                        // When pressed, update currentSelection with the target word.
                        currentSelection = targetWords[k];
                        selectedCells.clear();
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }
                ImGui::NewLine();
            }
        }

        // Popup Modal: Puzzle Completed (all target words found)
        if (!targetWords.empty() &&
            std::all_of(foundFlags.begin(), foundFlags.end(), [](bool b) { return b; })) {
            if (!puzzleCompletedPopupShown) {
                ImGui::OpenPopup("Puzzle Completed");
                puzzleCompletedPopupShown = true;
            }
            if (ImGui::BeginPopupModal("Puzzle Completed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Congratulations! You found all words!");
                ImGui::Spacing();
                if (ImGui::Button("Save & Exit                               ", ImVec2(250, 30))) {
                    solver.saveGridToFile("puzzle_output.txt");
                    window.close();
                }
                ImGui::NewLine();
                if (ImGui::Button("Back to Main Menu                         ", ImVec2(300, 30))) {
                    modeSelected = false;
                    selectedGridSize = 0;
                    targetWords.clear();
                    foundFlags.clear();
                    foundWordColors.clear();
                    currentSelection.clear();
                    selectedCells.clear();
                    usedCells.clear();
                    usedCellColors.clear();
                    solver = WordSearchSolver(); // Reinitialize the solver.
                    puzzleCompletedPopupShown = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End(); // End full-screen window.
        window.clear(sf::Color::White);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
