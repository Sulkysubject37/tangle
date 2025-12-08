#pragma once

#include <string>
#include "tangle/graph.hpp"

namespace tangle {
namespace io {

// Loads a graph from a BioGRID tab-separated file.
//
// @param filepath Path to the BioGRID file.
// @param use_entrez_id If true, uses Entrez Gene IDs (columns 2, 3). 
//                      If false, uses Official Symbols (columns 8, 9).
graph::PpiGraph load_from_biogrid(const std::string& filepath, bool use_entrez_id = false);

} // namespace io
} // namespace tangle
