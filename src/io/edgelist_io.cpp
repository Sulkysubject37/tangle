#include "tangle/io/edgelist_io.hpp"
#include <sstream>
#include <stdexcept>
#include <string>

namespace tangle {
namespace io {

graph::PpiGraph load_edgelist(const std::string& filepath, bool weighted, char delimiter) {
    graph::PpiGraph graph;
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open edgelist file: " + filepath);
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') { // Skip empty lines and comments
            continue;
        }

        std::stringstream ss(line);
        std::string u_str, v_str;
        Weight weight = 1.0;

        if (!std::getline(ss, u_str, delimiter)) {
            throw std::runtime_error("Malformed edgelist line: " + line);
        }
        if (!std::getline(ss, v_str, delimiter)) {
            throw std::runtime_error("Malformed edgelist line: " + line);
        }

        if (weighted) {
            std::string weight_str;
            if (!std::getline(ss, weight_str, delimiter)) {
                throw std::runtime_error("Weighted edgelist expects weight, but none found: " + line);
            }
            try {
                weight = std::stod(weight_str);
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid weight format in edgelist: " + weight_str);
            }
        }

        NodeId u_id = graph.get_or_add_node(u_str);
        NodeId v_id = graph.get_or_add_node(v_str);
        graph.add_edge(u_id, v_id, weight);
    }
    return graph;
}

void save_edgelist(const graph::PpiGraph& graph, const std::string& filepath, bool weighted, char delimiter) {
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        throw std::runtime_error("Could not open file for writing edgelist: " + filepath);
    }

    for (const auto& edge : graph.edges()) {
        outfile << graph.node(edge.u).protein_id << delimiter
                << graph.node(edge.v).protein_id;
        if (weighted) {
            outfile << delimiter << edge.weight;
        }
        outfile << "\n";
    }
}

} // namespace io
} // namespace tangle
