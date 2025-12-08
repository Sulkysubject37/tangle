#pragma once

#include "tangle/types.hpp"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace tangle {
namespace annotate {

using GoTermId = std::string;

// Represents a database of protein-to-GO-term annotations.
class AnnotationDb {
public:
  AnnotationDb() = default;

  // Loads annotations from a Gene Ontology Annotation (GOA) file.
  // The GAF format is expected (tab-separated).
  // We primarily care about column 2 (DB Object ID -> ProteinId) and 5 (GO ID).
  void load_from_gaf(const std::string &filepath);

  // Returns the set of GO terms associated with a given protein.
  // returned vector is sorted and unique.
  const std::vector<GoTermId> &get_annotations(const ProteinId &protein) const;

  // Returns all unique GO terms in the database.
  std::vector<GoTermId> get_all_go_terms() const;

  // Returns all proteins that have at least one annotation.
  std::vector<ProteinId> get_all_annotated_proteins() const;

  // Returns true if a protein has any annotations.
  bool has_annotations(const ProteinId &protein) const;

  // Returns the number of proteins annotated with the given GO term.
  // This is O(1) lookup.
  int get_term_frequency(const GoTermId &go_term) const;

private:
  // Maps ProteinId -> Sorted Vector of GO Term IDs
  std::unordered_map<ProteinId, std::vector<GoTermId>> protein_to_go_;

  // A sorted vector of all unique GO terms present in the database.
  std::vector<GoTermId> all_go_terms_;

  // Maps GoTermId -> Count of proteins annotated with this term
  std::unordered_map<GoTermId, int> term_counts_;
};

} // namespace annotate
} // namespace tangle
