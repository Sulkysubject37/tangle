#include "tangle/io/string_importer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <iostream>

namespace tangle {
namespace io {

graph::PpiGraph load_from_string(const std::string& filepath, double min_score, int score_column, char delimiter) {
    graph::PpiGraph graph;
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open STRING file: " + filepath);
    }

    std::string line;
    // Try to skip header, but it's not guaranteed to be correct.
    std::getline(infile, line);

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> columns;
        while(std::getline(ss, item, delimiter)) {
            columns.push_back(item);
        }

        if (columns.size() < score_column) {
            continue;
        }

        const std::string& protein1 = columns[0];
        const std::string& protein2 = columns[1];
        double score = 0.0;
        try {
            score = std::stod(columns[score_column - 1]);
        } catch(const std::exception& e) {
            continue;
        }

        if (score >= min_score) {
            NodeId u = graph.get_or_add_node(protein1);
            NodeId v = graph.get_or_add_node(protein2);
            graph.add_edge(u, v, score);
        }
    }

    if (graph.num_nodes() == 0) {
        // Don't throw, as it could be a valid empty graph
    }

    return graph;
}

} // namespace io
} // namespace tangle