#include "tangle/annotate/go_enrichment.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace tangle {
namespace annotate {

// Helper for combinations (n choose k) - can overflow with large numbers.
// A production-ready version would use log-gamma functions for stability.
long double combinations(int n, int k) {
  if (k < 0 || k > n) {
    return 0;
  }
  if (k == 0 || k == n) {
    return 1;
  }
  if (k > n / 2) {
    k = n - k;
  }
  long double res = 1;
  for (int i = 1; i <= k; ++i) {
    res = res * (n - i + 1) / i;
  }
  return res;
}

// Hypergeometric probability mass function: P(X=k)
long double hypergeometric_pmf(int k, int n, int K, int N) {
  return combinations(K, k) * combinations(N - K, n - k) / combinations(N, n);
}

// Hypergeometric cumulative distribution function: P(X >= k)
// This is the p-value (right-tailed test).
long double hypergeometric_cdf_upper(int k, int n, int K, int N) {
  long double p = 0;
  for (int i = k; i <= std::min(n, K); ++i) {
    p += hypergeometric_pmf(i, n, K, N);
  }
  return p;
}

std::vector<GoEnrichmentResult>
go_enrichment(const std::vector<ProteinId> &protein_set, const AnnotationDb &db,
              const std::string &correction_method) {
  std::vector<GoEnrichmentResult> results;

  // N: total number of proteins in the background
  const unsigned int N = db.get_all_annotated_proteins().size();
  if (N == 0)
    return results;

  // n: total number of proteins in the query set
  const unsigned int n = protein_set.size();
  if (n == 0)
    return results;

  // 1. Identify unique GO terms in the query set and count 'k' (count in set)
  std::map<GoTermId, int> unique_terms_in_set;
  for (const auto &prot : protein_set) {
    const auto &terms = db.get_annotations(prot);
    for (const auto &term : terms) {
      unique_terms_in_set[term]++;
    }
  }

  // 2. Iterate ONLY over the terms present in the query set
  for (const auto &pair : unique_terms_in_set) {
    const GoTermId &go_term = pair.first;
    int k = pair.second; // count in set (already computed)

    // Get K (count in background) - O(1) lookup
    int K = db.get_term_frequency(go_term);

    // Perform HGT
    if (k > 0) { // Should always be true here
      double p_value =
          static_cast<double>(hypergeometric_cdf_upper(k, n, K, N));

      GoEnrichmentResult res = {go_term,
                                p_value,
                                0.0, // adjusted p-value
                                static_cast<unsigned int>(k),
                                n,
                                static_cast<unsigned int>(K),
                                N};
      results.push_back(res);
    }
  }

  // Apply multiple testing correction
  if (!results.empty()) {
    // Only sort if we need to output in specific order, but usually caller
    // handles that. Correction based on number of tests performed. Note:
    // Technically Bonferroni should correct for ALL possible hypotheses
    // (all_go_terms.size()), but often in enrichment tools it's corrected
    // against the number of *tested* hypotheses. If strict Bonferroni against
    // entire DB is desired, use db.get_all_go_terms().size(). Standard tools
    // often use the tested set or Benjamini-Hochberg. For strict correctness
    // adhering to "we tested these terms", we use results.size().

    size_t num_tests = results.size();

    if (correction_method == "bonferroni") {
      for (auto &res : results) {
        res.adjusted_p_value = std::min(1.0, res.p_value * num_tests);
      }
    } else {
      for (auto &res : results) {
        res.adjusted_p_value = res.p_value;
      }
    }
  }

  return results;
}

} // namespace annotate
} // namespace tangle
