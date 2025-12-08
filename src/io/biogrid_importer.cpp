#include "tangle/io/biogrid_importer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace tangle {
namespace io {

graph::PpiGraph load_from_biogrid(const std::string& filepath, bool use_entrez_id) {
    graph::PpiGraph graph;
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open BioGRID file: " + filepath);
    }

    std::string line;
    // Skip header
    if (!std::getline(infile, line) || line.empty() || line[0] != '#') {
        // This is not a standard biogrid file, but we can try to parse it anyway
        infile.seekg(0);
    }

    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> columns;
        while(std::getline(ss, item, '\t')) {
            columns.push_back(item);
        }

        // BioGRID format has many columns. We need at least 9 for official symbols.
        if (columns.size() < 9) continue;

        const std::string& protein1 = use_entrez_id ? columns[1] : columns[7];
        const std::string& protein2 = use_entrez_id ? columns[2] : columns[8];

        if (protein1 == "-" || protein2 == "-") continue;

        NodeId u = graph.get_or_add_node(protein1);
        NodeId v = graph.get_or_add_node(protein2);
        graph.add_edge(u, v);
    }

    if (graph.num_nodes() == 0) {
        throw std::runtime_error("No nodes were loaded from the BioGRID file. Check file format.");
    }

    return graph;
}

} // namespace io
} // namespace tangle
