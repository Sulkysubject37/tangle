#pragma once

#include <string>
#include "tangle/graph.hpp"

namespace tangle {
namespace io {

// Loads a graph from a STRING-db links file.
// Assumes a space-separated format with protein1, protein2, and scores.
//
// @param filepath Path to the STRING links file.
// @param min_score The minimum combined score to include an edge.
// @param score_column The 1-based index of the column containing the score.
//                     For STRING `protein.links.vX.Y.txt`, the `combined_score`
//                     is typically the last column (e.g., column 10).
//                     However, since the file format is space-separated, we
//                     assume it's the last column we care about. For simplicity,
//                     this implementation just reads words, so column indices are tricky.
//                     Let's use a simpler approach for now.
graph::PpiGraph load_from_string(const std::string& filepath, double min_score = 700.0, int score_column = 10, char delimiter = ' ');

} // namespace io
} // namespace tangle
