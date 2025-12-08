#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include "tangle/types.hpp"

namespace tangle {
namespace graph {

struct Node {
    NodeId id;
    ProteinId protein_id;
    std::optional<GeneSymbol> gene_symbol;
};

struct Edge {
    NodeId u;
    NodeId v;
    Weight weight;
};

class PpiGraph {
public:
    PpiGraph() = default;

    NodeId add_node(const ProteinId& pid,
                    const std::optional<GeneSymbol>& symbol = std::nullopt);

    NodeId get_or_add_node(const ProteinId& pid,
                           const std::optional<GeneSymbol>& symbol = std::nullopt);

    void add_edge(NodeId u, NodeId v, Weight w = 1.0);

    const Node& node(NodeId id) const;
    const std::vector<Node>& nodes() const { return nodes_; }
    const std::vector<Edge>& edges() const { return edges_; }

    const std::vector<NodeId>& neighbors(NodeId id) const;

    std::size_t num_nodes() const { return nodes_.size(); }
    std::size_t num_edges() const { return edges_.size(); }

    std::optional<NodeId> find_node(const ProteinId& pid) const;

private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::vector<NodeId>> adj_;
    std::unordered_map<ProteinId, NodeId> protein_index_;

    NodeId add_node_internal(const ProteinId& pid,
                             const std::optional<GeneSymbol>& symbol);
    void ensure_adj_size();

};

} // namespace graph
} // namespace tangle
