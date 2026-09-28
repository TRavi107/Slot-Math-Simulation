#pragma once
#include <OpenXLSX.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "constants.hpp"

namespace ReelFunctions {

    inline void get_reel_data(const char* sheetName, const char* excelFilePath,
                              std::vector<std::vector<GameSymbols>>& reel,
                              int cols, int startCol = 0)
    {
        if (!excelFilePath || excelFilePath[0] == '\0') {
            std::printf("[get_reel_data] ERROR: excelFilePath is null or empty\n");
            throw std::invalid_argument("get_reel_data: excelFilePath is null or empty");
        }

        OpenXLSX::XLDocument doc;
        try {
            doc.open(excelFilePath);
        }
        catch (const std::exception& e) {
            std::printf("[get_reel_data] ERROR: failed to open '%s' - %s\n", excelFilePath, e.what());
            throw std::runtime_error("get_reel_data: failed to open '" + std::string(excelFilePath) + "' - " + e.what());
        }

        auto wb = doc.workbook();
        if (!wb.worksheetExists(sheetName)) {
            doc.close();
            std::printf("[get_reel_data] ERROR: worksheet '%s' not found in '%s'\n", sheetName, excelFilePath);
            throw std::runtime_error("get_reel_data: worksheet '" + std::string(sheetName) + "' not found");
        }

        auto ws = wb.worksheet(sheetName);
        const uint32_t lastRow = ws.rowCount();   // OpenXLSX rows/cols are 1-based

        reel.assign(cols, {});

        for (int c = 0; c < cols; ++c) {
            auto& strip = reel[c];
            strip.reserve(lastRow);
            const auto excelCol = static_cast<uint16_t>(startCol + c + 1);

            for (uint32_t row = 1; row <= lastRow; ++row) {
                OpenXLSX::XLCellValue value = ws.cell(row, excelCol).value();
                if (value.type() != OpenXLSX::XLValueType::String)
                    continue;

                std::string s = value.get<std::string>();

                // trim surrounding whitespace
                s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                        [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(),
                        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

                GameSymbols sym = Constants::gameSymbolFromString(s.c_str());
                if (sym == GameSymbols::Invalid)
                    continue;

                strip.push_back(sym);
            }

            if (strip.empty()) {
                doc.close();
                throw std::runtime_error("get_reel_data: reel " + std::to_string(c) +
                    " in sheet '" + std::string(sheetName) + "' is empty");
            }
        }

        doc.close();
    }

    inline void generate_slot_matrix(const std::vector<std::vector<GameSymbols>>& reelsVector,
                                     const int matrixSize[2],
                                     std::vector<std::vector<GameSymbols>>& matrix) {
        // Thread-local RNG — safe for multi-threaded simulation
        thread_local std::mt19937 rng(std::random_device{}());

        for (int col = 0; col < matrixSize[1]; ++col) {
            const std::vector<GameSymbols>& reel = reelsVector[col];

            std::uniform_int_distribution<int> dist(0, static_cast<int>(reel.size()) - 1);
            int startIdx = dist(rng);

            for (int row = 0; row < matrixSize[0]; row++)
                matrix[row][col] = reel[(startIdx + row) % reel.size()];
        }
    }

    inline GameSymbols get_random_from_Col(const int colIndex,
                                           const std::vector<std::vector<GameSymbols>>& reelsVector) {
        thread_local std::mt19937 rng(std::random_device{}());
        const std::vector<GameSymbols>& reel = reelsVector[colIndex];

        std::uniform_int_distribution<int> dist(0, static_cast<int>(reel.size()) - 1);
        return reel[dist(rng)];
    }
}