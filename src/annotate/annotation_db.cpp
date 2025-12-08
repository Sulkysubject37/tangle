#include "tangle/annotate/annotation_db.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tangle {
namespace annotate {

void AnnotationDb::load_from_gaf(const std::string &filepath) {
  std::ifstream infile(filepath);
  if (!infile.is_open()) {
    throw std::runtime_error("Could not open GAF file: " + filepath);
  }

  protein_to_go_.clear();
  all_go_terms_.clear();

  // Temporary storage for all GO terms to allow bulk sorting/uniquing later if
  // needed, but for all_go_terms_ we'll just insert and sort at the end.

  std::string line;
  // Reserve reasonable space to avoid reallocations if possible
  // Estimating lines is hard without seeking, so rely on vector growth.

  while (std::getline(infile, line)) {
    if (line.empty() || line[0] == '!') { // Skip empty lines and comments
      continue;
    }

    // Optimized parsing using find/substr to avoid stringstream overhead
    size_t pos = 0;
    int col_idx = 0;
    std::string protein_id;
    std::string protein_symbol;
    std::string go_id;
    bool found_prot = false;
    bool found_symbol = false;
    bool found_go = false;

    // We need column 2 (index 1) for ProteinId, column 3 (index 2) for Symbol,
    // and column 5 (index 4) for GO ID.
    // Columns are tab-separated.

    size_t next_pos;
    while ((next_pos = line.find('\t', pos)) != std::string::npos) {
      if (col_idx == 1) {
        protein_id = line.substr(pos, next_pos - pos);
        found_prot = true;
      } else if (col_idx == 2) {
        protein_symbol = line.substr(pos, next_pos - pos);
        found_symbol = true;
      } else if (col_idx == 4) {
        go_id = line.substr(pos, next_pos - pos);
        found_go = true;
        break; // We have what we need
      }
      pos = next_pos + 1;
      col_idx++;
    }

    // Handle case where GO ID might be the last column
    if (!found_go && col_idx == 4) {
      go_id = line.substr(pos);
      found_go = true;
    }

    if (found_prot && found_go && !go_id.empty()) {
      // We don't check for uniqueness here to avoid O(N) search on every
      // insert. We will sort and unique later.
      protein_to_go_[protein_id].push_back(go_id);
      all_go_terms_.push_back(go_id);

      // Dual Indexing: Note that Symbols are not always unique, but for
      // enrichment it's better to be permissive.
      if (found_symbol && !protein_symbol.empty()) {
        protein_to_go_[protein_symbol].push_back(go_id);
      }
    }
  }

  // Post-process: Sort and Unique
  term_counts_.clear();
  for (auto &pair : protein_to_go_) {
    std::sort(pair.second.begin(), pair.second.end());
    pair.second.erase(std::unique(pair.second.begin(), pair.second.end()),
                      pair.second.end());

    // Count frequencies after uniquing for this protein
    for (const auto &term : pair.second) {
      term_counts_[term]++;
    }
  }

  std::sort(all_go_terms_.begin(), all_go_terms_.end());
  all_go_terms_.erase(std::unique(all_go_terms_.begin(), all_go_terms_.end()),
                      all_go_terms_.end());
}

const std::vector<GoTermId> &
AnnotationDb::get_annotations(const ProteinId &protein) const {
  static const std::vector<GoTermId> empty_vec;
  auto it = protein_to_go_.find(protein);
  if (it != protein_to_go_.end()) {
    return it->second;
  }
  return empty_vec;
}

int AnnotationDb::get_term_frequency(const GoTermId &go_term) const {
  auto it = term_counts_.find(go_term);
  if (it != term_counts_.end()) {
    return it->second;
  }
  return 0;
}

std::vector<GoTermId> AnnotationDb::get_all_go_terms() const {
  return all_go_terms_;
}

std::vector<ProteinId> AnnotationDb::get_all_annotated_proteins() const {
  std::vector<ProteinId> proteins;
  proteins.reserve(protein_to_go_.size());
  for (const auto &pair : protein_to_go_) {
    proteins.push_back(pair.first);
  }
  return proteins;
}

bool AnnotationDb::has_annotations(const ProteinId &protein) const {
  return protein_to_go_.count(protein) > 0;
}

} // namespace annotate
} // namespace tangle
