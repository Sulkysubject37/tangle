#include "tangle/algo/community.hpp"
#include <numeric>
#include <algorithm>
#include <map>
#include <vector>
#include <random>
#include <set>

namespace tangle {
namespace algo {

// A more robust implementation of the Louvain method, guided by best practices.
// This version focuses on a correct Phase 1 implementation.

// Helper to calculate modularity gain. This is a critical part.
// ΔQ = [ (k_i_in / 2m) - ( (Σ_tot * k_i) / (2m)^2 ) ]
// We calculate the gain of moving node `u` into community `C`.
double calculate_modularity_gain(
    NodeId u,
    NodeId target_community_id,
    double k_i, // degree of node u
    const std::vector<NodeId>& communities,
    const std::vector<double>& community_total_degrees, // Σ_tot for each community
    const graph::PpiGraph& graph,
    double m // total number of edges
) {
    double k_i_in = 0.0;
    for (const auto& neighbor : graph.neighbors(u)) {
        if (communities[neighbor] == target_community_id) {
            k_i_in += 1.0; // Assuming unweighted for now.
        }
    }
    
    double sigma_tot = community_total_degrees.at(target_community_id);
    double delta_q = k_i_in - (sigma_tot * k_i) / (2.0 * m);
    return delta_q;
}

std::vector<std::vector<NodeId>> louvain_community(const graph::PpiGraph& graph, bool use_weights) {
    if (graph.num_nodes() == 0) {
        return {};
    }

    // --- Initialization ---
    // communities[i] = community ID of node i
    std::vector<NodeId> communities(graph.num_nodes());
    std::iota(communities.begin(), communities.end(), 0); 

    std::vector<double> node_degrees(graph.num_nodes());
    double m = 0.0; // Total number of edges (sum of weights / 2)

    for (const auto& edge : graph.edges()) {
        double weight = use_weights ? edge.weight : 1.0;
        node_degrees[edge.u] += weight;
        node_degrees[edge.v] += weight;
        m += weight;
    }
    m /= 2.0;

    if (m == 0.0) { // No edges
        std::vector<std::vector<NodeId>> result;
        for(NodeId i = 0; i < graph.num_nodes(); ++i) result.push_back({i});
        return result;
    }

    // --- Main Loop: Repeat until no more improvement ---
    bool improvement = true;
    while (improvement) {
        improvement = false;

        // --- Phase 1: Modularity Optimization ---
        
        // Randomize node order to avoid getting stuck
        std::vector<NodeId> node_order(graph.num_nodes());
        std::iota(node_order.begin(), node_order.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(node_order.begin(), node_order.end(), g);
        
        for (NodeId u : node_order) {
            NodeId original_community = communities[u];
            NodeId best_community = original_community;
            double max_gain = 0.0;

            // Calculate sum of degrees for each community
            std::map<NodeId, double> community_degrees_map;
            for(size_t i=0; i<graph.num_nodes(); ++i){
                community_degrees_map[communities[i]] += node_degrees[i];
            }

            // Consider moving to neighbor communities
            std::set<NodeId> neighbor_communities;
            for (const auto& neighbor : graph.neighbors(u)) {
                neighbor_communities.insert(communities[neighbor]);
            }

            for (NodeId target_community : neighbor_communities) {
                if (target_community == original_community) continue;
                
                // Calculate modularity gain for moving u to target_community
                double k_i_in = 0.0;
                for (const auto& neighbor : graph.neighbors(u)) {
                    if (communities[neighbor] == target_community) {
                        k_i_in += 1.0; // unweighted for now
                    }
                }
                
                double sigma_tot = community_degrees_map[target_community];
                double k_i = node_degrees[u];
                
                double gain = k_i_in - (sigma_tot * k_i) / (2.0 * m);

                if (gain > max_gain) {
                    max_gain = gain;
                    best_community = target_community;
                }
            }

            // If a move is beneficial, make it
            if (best_community != original_community) {
                // Move node u to the best community
                communities[u] = best_community;
                improvement = true;
            }
        }
    }
    
    // --- Phase 2: Community Aggregation (simplified for now) ---
    // The current implementation only performs one level of community detection.
    // A full implementation would create a new graph where communities are nodes
    // and repeat the process. We will stick to one level for this version.

    // Map communities to a dense set of community IDs
    std::map<NodeId, std::vector<NodeId>> community_map;
    for (size_t i = 0; i < communities.size(); ++i) {
        community_map[communities[i]].push_back(i);
    }
    
    std::vector<std::vector<NodeId>> result;
    for (auto const& [key, val] : community_map) {
        result.push_back(val);
    }
    
    return result;
}

} // namespace algo
} // namespace tangle