#include "tangle/algo/centrality.hpp"
#include <stdexcept>

namespace tangle {
namespace algo {

std::vector<double> degree_centrality(const graph::PpiGraph& graph, bool use_weights) {
    std::vector<double> degrees(graph.num_nodes(), 0.0);

    if (use_weights) {
        for (const auto& edge : graph.edges()) {
            degrees[edge.u] += edge.weight;
            degrees[edge.v] += edge.weight; // Undirected graph
        }
    } else {
        for (NodeId i = 0; i < graph.num_nodes(); ++i) {
            degrees[i] = static_cast<double>(graph.neighbors(i).size());
        }
    }
    return degrees;
}

} // namespace algo
} // namespace tangle
