#include "tangle/graph.hpp"
#include <stdexcept> // For std::out_of_range

namespace tangle {
namespace graph {

NodeId PpiGraph::add_node_internal(const ProteinId& pid,
                                   const std::optional<GeneSymbol>& symbol) {
    NodeId id = static_cast<NodeId>(nodes_.size());
    Node n{ id, pid, symbol };
    nodes_.push_back(n);
    protein_index_[pid] = id;
    ensure_adj_size();
    return id;
}

NodeId PpiGraph::add_node(const ProteinId& pid,
                          const std::optional<GeneSymbol>& symbol) {
    if (auto it = protein_index_.find(pid); it != protein_index_.end()) {
        return it->second;
    }
    return add_node_internal(pid, symbol);
}

NodeId PpiGraph::get_or_add_node(const ProteinId& pid,
                                 const std::optional<GeneSymbol>& symbol) {
    auto it = protein_index_.find(pid);
    if (it != protein_index_.end()) {
        return it->second;
    }
    return add_node_internal(pid, symbol);
}

void PpiGraph::add_edge(NodeId u, NodeId v, Weight w) {
    if (u >= nodes_.size() || v >= nodes_.size()) {
        throw std::out_of_range("NodeId out of range in add_edge");
    }
    edges_.push_back(Edge{u, v, w});
    ensure_adj_size(); // Ensure adj_ is large enough for the new node indices
    adj_[u].push_back(v);
    adj_[v].push_back(u); // undirected
}

const Node& PpiGraph::node(NodeId id) const {
    if (id >= nodes_.size()) {
        throw std::out_of_range("NodeId out of range in node()");
    }
    return nodes_[id];
}

const std::vector<NodeId>& PpiGraph::neighbors(NodeId id) const {
    if (id >= adj_.size()) {
        throw std::out_of_range("NodeId out of range in neighbors()");
    }
    return adj_[id];
}

std::optional<NodeId> PpiGraph::find_node(const ProteinId& pid) const {
    auto it = protein_index_.find(pid);
    if (it == protein_index_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PpiGraph::ensure_adj_size() {
    if (adj_.size() < nodes_.size()) {
        adj_.resize(nodes_.size());
    }
}

} // namespace graph
} // namespace tangle
