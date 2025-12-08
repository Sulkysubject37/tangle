#pragma once

#include <string>
#include "tangle/graph.hpp"

namespace tangle {
namespace export_ {

// Exports a PpiGraph to an SBML file.
// This is a minimal implementation that creates a basic SBML Level 2 Version 5 file.
// - Proteins are represented as <species>.
// - Interactions are represented as <reaction> elements with the two proteins as reactants.
//
// @param graph The graph to export.
// @param filepath The path to the output SBML file.
void save_to_sbml(const graph::PpiGraph& graph, const std::string& filepath);

} // namespace export_
} // namespace tangle
