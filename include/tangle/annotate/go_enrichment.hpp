#pragma once

#include <string>
#include <vector>
#include <cmath> // For std::log10
#include "tangle/annotate/annotation_db.hpp"

namespace tangle {
namespace annotate {

// Represents the result of a GO enrichment analysis for a single term.
struct GoEnrichmentResult {
    GoTermId go_term;
    double p_value;
    double adjusted_p_value;
    unsigned int count_in_set; // k: number of proteins in the query set with this term
    unsigned int total_in_set; // n: total number of proteins in the query set
    unsigned int count_in_background; // K: number of proteins in the background with this term
    unsigned int total_in_background; // N: total number of proteins in the background
};

// Performs Gene Ontology (GO) enrichment analysis on a set of proteins.
// Uses the hypergeometric test to determine over-representation of GO terms.
//
// @param protein_set A vector of ProteinIds to analyze (e.g., a community).
// @param db The annotation database to use as a background and for term lookups.
// @param correction_method The multiple testing correction method to apply.
//                          "bonferroni" is a simple, strict method.
//                          "none" will not apply any correction.
// @return A vector of enrichment results, typically filtered for significance.
std::vector<GoEnrichmentResult> go_enrichment(
    const std::vector<ProteinId>& protein_set,
    const AnnotationDb& db,
    const std::string& correction_method = "bonferroni"
);


} // namespace annotate
} // namespace tangle
