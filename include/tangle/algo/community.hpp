#pragma once

#include <vector>
#include "tangle/graph.hpp"

namespace tangle {
namespace algo {

// Performs community detection using a simplified Louvain method.
// Returns a vector of vectors, where each inner vector is a community of NodeIds.
// Note: This is a complex algorithm, and this implementation is a simplified
// starting point, focusing on the first phase of modularity optimization.
std::vector<std::vector<NodeId>> louvain_community(const graph::PpiGraph& graph, bool use_weights = false);

} // namespace algo
} // namespace tangle
