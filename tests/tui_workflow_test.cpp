#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "tangle/algo/community.hpp"
#include "tangle/annotate/annotation_db.hpp"
#include "tangle/annotate/go_enrichment.hpp"
#include "tangle/export/sbml_exporter.hpp"
#include "tangle/graph.hpp"
#include "tangle/io/edgelist_io.hpp"
#include "tangle/io/string_importer.hpp"

// Re-defining AppState here to test it independently of the FTXUI code.
struct AppState {
  std::string edgelist_path;
  std::string string_path;
  std::string gaf_path;
  std::string sbml_path;

  std::unique_ptr<tangle::graph::PpiGraph> graph;
  std::unique_ptr<tangle::annotate::AnnotationDb> annotation_db;
  std::vector<std::vector<tangle::NodeId>> communities;
  std::vector<tangle::annotate::GoEnrichmentResult> enrichment_results;

  int node_count = 0;
  int edge_count = 0;
  int community_count = 0;
  std::string min_score_str = "700";
  std::string score_col_str = "10";
  int delimiter_choice = 0; // 0=Space, 1=Tab

  std::string status_message = "Ready.";

  void load_edgelist_graph() {
    if (edgelist_path.empty())
      return;
    graph = std::make_unique<tangle::graph::PpiGraph>(
        tangle::io::load_edgelist(edgelist_path, false));
    node_count = graph->num_nodes();
    edge_count = graph->num_edges();
  }

  void import_from_string() {
    if (string_path.empty())
      return;
    double min_score = std::stod(min_score_str);
    int score_col = std::stoi(score_col_str);
    char delimiter = (delimiter_choice == 0) ? ' ' : '\t';
    graph =
        std::make_unique<tangle::graph::PpiGraph>(tangle::io::load_from_string(
            string_path, min_score, score_col, delimiter));
    node_count = graph->num_nodes();
    edge_count = graph->num_edges();
  }

  void run_louvain() {
    if (!graph)
      return;
    communities = tangle::algo::louvain_community(*graph);
    community_count = communities.size();
  }

  void load_annotations() {
    if (gaf_path.empty())
      return;
    annotation_db = std::make_unique<tangle::annotate::AnnotationDb>();
    annotation_db->load_from_gaf(gaf_path);
  }

  void run_go_enrichment() {
    if (communities.empty() || !annotation_db || !graph)
      return;
    enrichment_results.clear();
    for (const auto &comm : communities) {
      std::vector<tangle::ProteinId> protein_set;
      for (const auto &node_id : comm) {
        protein_set.push_back(graph->node(node_id).protein_id);
      }
      auto results =
          tangle::annotate::go_enrichment(protein_set, *annotation_db);
      for (const auto &res : results) {
        if (res.adjusted_p_value <= 1.0) { // Keep all results for test
          enrichment_results.push_back(res);
        }
      }
    }
  }

  void export_to_sbml() {
    if (!graph)
      return;
    if (sbml_path.empty())
      sbml_path = "output.sbml";
    tangle::export_::save_to_sbml(*graph, sbml_path);
  }
};

TEST_CASE("TUI Workflow Integration Test", "[tui_workflow]") {
  AppState state;
  state.string_path = "../tests/dummy_string.txt";
  state.gaf_path = "../tests/dummy_goa.gaf";
  state.sbml_path = "test_workflow_output.sbml";

  SECTION("Full Workflow (Standard STRING)") {
    // 1. Import from STRING
    state.import_from_string();
    REQUIRE(state.graph != nullptr);
    REQUIRE(state.node_count > 0);
    REQUIRE(state.edge_count > 0);

    // ... rest of the workflow ...
    state.run_louvain();
    REQUIRE_FALSE(state.communities.empty());

    state.load_annotations();
    REQUIRE(state.annotation_db != nullptr);

    state.run_go_enrichment();
    REQUIRE_FALSE(state.enrichment_results.empty());

    state.export_to_sbml();
    std::ifstream sbml_file(state.sbml_path);
    REQUIRE(sbml_file.is_open());

    // Cleanup
    sbml_file.close();
    std::remove(state.sbml_path.c_str());
  }

  SECTION("STRING with Custom Column (Simulating test_case.tsv)") {
    // Create a dummy custom STRING file
    std::string custom_string_path = "custom_string_test.tsv";
    std::ofstream outfile(custom_string_path);
    // Header (13 columns)
    outfile << "c1\tc2\tc3\tc4\tc5\tc6\tc7\tc8\tc9\tc10\tc11\tc12\tscore\n";
    // Data: p1, p2, ..., 0.95
    outfile << "p1\tp2\t.\t.\t.\t.\t.\t.\t.\t.\t.\t.\t0.95\n";
    outfile << "p2\tp3\t.\t.\t.\t.\t.\t.\t.\t.\t.\t.\t0.90\n";
    outfile.close();

    state.string_path = custom_string_path;
    state.score_col_str = "13";
    state.delimiter_choice = 1; // Tab
    state.min_score_str = "0.7";

    state.import_from_string();

    REQUIRE(state.graph != nullptr);
    REQUIRE(state.node_count == 3); // p1, p2, p3
    REQUIRE(state.edge_count == 2);

    // Cleanup
    std::remove(custom_string_path.c_str());
  }
}
