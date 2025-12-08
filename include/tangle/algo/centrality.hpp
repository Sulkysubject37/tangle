#pragma once

#include <vector>
#include <numeric> // For std::accumulate
#include "tangle/graph.hpp"

namespace tangle {
namespace algo {

// Computes the degree centrality for each node in the graph.
// For unweighted graphs, this is the number of edges connected to each node.
// For weighted graphs, if `use_weights` is true, it's the sum of the weights of connected edges.
std::vector<double> degree_centrality(const graph::PpiGraph& graph, bool use_weights = false);

} // namespace algo
} // namespace tangle
