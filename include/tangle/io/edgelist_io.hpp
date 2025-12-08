#pragma once
#include <string>
#include <vector>
#include <fstream> // Required for file operations
#include "tangle/graph.hpp"

namespace tangle {
namespace io {

// Function to load a graph from an edgelist file.
// Edgelist format: each line contains two node IDs (ProteinId) and optionally a weight,
// separated by the specified delimiter.
// If weighted is true, expects three columns: u, v, weight.
// If weighted is false, expects two columns: u, v, and assigns default weight 1.0.
graph::PpiGraph load_edgelist(const std::string& filepath, bool weighted = false, char delimiter = '\t');

// Function to save a graph to an edgelist file.
// Writes each edge to a new line, with node IDs and optionally weight,
// separated by the specified delimiter.
void save_edgelist(const graph::PpiGraph& graph, const std::string& filepath, bool weighted = false, char delimiter = '\t');

} // namespace io
} // namespace tangle
