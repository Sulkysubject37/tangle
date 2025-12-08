#include "tangle/export/sbml_exporter.hpp"
#include <fstream>
#include <stdexcept>

namespace tangle {
namespace export_ {

void save_to_sbml(const graph::PpiGraph& graph, const std::string& filepath) {
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        throw std::runtime_error("Could not open SBML file for writing: " + filepath);
    }

    outfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    outfile << "<sbml xmlns=\"http://www.sbml.org/sbml/level2/version5\" level=\"2\" version=\"5\">\n";
    outfile << "  <model id=\"tangle_ppi_network\">\n";
    
    outfile << "    <listOfCompartments>\n";
    outfile << "      <compartment id=\"default\" size=\"1\"/>\n";
    outfile << "    </listOfCompartments>\n";

    outfile << "    <listOfSpecies>\n";
    for (const auto& node : graph.nodes()) {
        outfile << "      <species id=\"" << node.protein_id << "\" name=\"" << node.protein_id << "\" compartment=\"default\" initialAmount=\"1\"/>\n";
    }
    outfile << "    </listOfSpecies>\n";

    outfile << "    <listOfReactions>\n";
    int reaction_count = 0;
    for (const auto& edge : graph.edges()) {
        outfile << "      <reaction id=\"r" << reaction_count++ << "\" reversible=\"false\">\n";
        outfile << "        <listOfReactants>\n";
        outfile << "          <speciesReference species=\"" << graph.node(edge.u).protein_id << "\"/>\n";
        outfile << "          <speciesReference species=\"" << graph.node(edge.v).protein_id << "\"/>\n";
        outfile << "        </listOfReactants>\n";
        outfile << "        <listOfProducts/>\n";
        outfile << "        <kineticLaw/>\n";
        outfile << "      </reaction>\n";
    }
    outfile << "    </listOfReactions>\n";

    outfile << "  </model>\n";
    outfile << "</sbml>\n";
}

} // namespace export_ 
} // namespace tangle
