#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch.hpp"
#include "tangle/algo/centrality.hpp"
#include "tangle/algo/community.hpp"
#include "tangle/annotate/annotation_db.hpp"
#include "tangle/annotate/go_enrichment.hpp"
#include "tangle/export/sbml_exporter.hpp"
#include "tangle/graph.hpp"
#include "tangle/io/edgelist_io.hpp"
#include "tangle/io/string_importer.hpp"
#include <algorithm> // For std::sort
#include <cstdio>    // For std::remove
#include <ctime>     // For std::time
#include <fstream>
#include <numeric>   // For std::iota, etc.
#include <streambuf> // For std::istreambuf_iterator
#include <string>
#include <vector> // For std::vector comparison

// Helper to create a temporary edgelist file (already present, just for
// context)
std::string create_temp_edgelist_file(
    const std::string &content,
    const std::string &filename_prefix = "test_edgelist_") {
  // Generate a unique filename (simple timestamp based for now)
  std::string filename =
      filename_prefix + std::to_string(std::time(nullptr)) + ".txt";
  std::ofstream outfile(filename);
  if (!outfile.is_open()) {
    throw std::runtime_error("Failed to create temporary file: " + filename);
  }
  outfile << content;
  outfile.close();
  return filename;
}

TEST_CASE("PpiGraph core functionality", "[graph]") {
  tangle::graph::PpiGraph g;
  auto a = g.get_or_add_node("P12345");
  auto b = g.get_or_add_node("Q99999");
  g.add_edge(a, b, 0.9);

  REQUIRE(g.num_nodes() == 2);
  REQUIRE(g.num_edges() == 1);
  REQUIRE_FALSE(g.neighbors(a).empty());
  REQUIRE(g.node(a).protein_id == "P12345");
  REQUIRE(g.node(b).protein_id == "Q99999");
}

TEST_CASE("Edgelist I/O functionality", "[io]") {
  // Test unweighted edgelist load
  std::string unweighted_content = "ProtA\tProtB\nProtB\tProtC\nProtC\tProtA\n";
  std::string unweighted_filepath =
      create_temp_edgelist_file(unweighted_content, "unweighted_edgelist_");

  tangle::graph::PpiGraph unweighted_graph =
      tangle::io::load_edgelist(unweighted_filepath);
  REQUIRE(unweighted_graph.num_nodes() == 3);
  REQUIRE(unweighted_graph.num_edges() == 3);
  REQUIRE(unweighted_graph.find_node("ProtA").has_value());
  REQUIRE(unweighted_graph.find_node("ProtB").has_value());
  REQUIRE(unweighted_graph.find_node("ProtC").has_value());
  std::remove(unweighted_filepath.c_str()); // Clean up temp file

  // Test weighted edgelist load
  std::string weighted_content =
      "ProtX\tProtY\t0.5\nProtY\tProtZ\t0.8\nProtZ\tProtX\t1.0\n";
  std::string weighted_filepath =
      create_temp_edgelist_file(weighted_content, "weighted_edgelist_");

  tangle::graph::PpiGraph weighted_graph =
      tangle::io::load_edgelist(weighted_filepath, true);
  REQUIRE(weighted_graph.num_nodes() == 3);
  REQUIRE(weighted_graph.num_edges() == 3);
  std::remove(weighted_filepath.c_str()); // Clean up temp file

  // Test save_edgelist and then load back
  std::string save_filepath = create_temp_edgelist_file(
      "", "saved_edgelist_"); // Create an empty file to save into
  tangle::io::save_edgelist(
      unweighted_graph,
      save_filepath); // Save the previously loaded unweighted graph

  tangle::graph::PpiGraph loaded_saved_graph =
      tangle::io::load_edgelist(save_filepath);
  REQUIRE(loaded_saved_graph.num_nodes() == unweighted_graph.num_nodes());
  REQUIRE(loaded_saved_graph.num_edges() == unweighted_graph.num_edges());

  // Check content by reading the saved file directly
  std::ifstream saved_file(save_filepath);
  std::string saved_line;
  int line_count = 0;
  while (std::getline(saved_file, saved_line)) {
    line_count++;
    REQUIRE(saved_line.find('\t') !=
            std::string::npos); // Basic check for expected format
  }
  REQUIRE(line_count == unweighted_graph.num_edges());

  std::remove(save_filepath.c_str()); // Clean up temp file
}

TEST_CASE("Degree Centrality algorithm", "[algo][centrality]") {
  SECTION("Unweighted graph") {
    // Create a simple graph: A-B, B-C, C-D
    tangle::graph::PpiGraph g;
    g.get_or_add_node("A");
    g.get_or_add_node("B");
    g.get_or_add_node("C");
    g.get_or_add_node("D");

    g.add_edge(g.find_node("A").value(), g.find_node("B").value(), 1.0);
    g.add_edge(g.find_node("B").value(), g.find_node("C").value(), 1.0);
    g.add_edge(g.find_node("C").value(), g.find_node("D").value(), 1.0);

    std::vector<double> degrees = tangle::algo::degree_centrality(g, false);

    // Expected degrees: A=1, B=2, C=2, D=1
    REQUIRE(degrees.size() == 4);
    REQUIRE(degrees[g.find_node("A").value()] == 1.0);
    REQUIRE(degrees[g.find_node("B").value()] == 2.0);
    REQUIRE(degrees[g.find_node("C").value()] == 2.0);
    REQUIRE(degrees[g.find_node("D").value()] == 1.0);
  }

  SECTION("Weighted graph") {
    // Create a simple weighted graph: A-B (0.5), B-C (1.5), C-A (2.0)
    tangle::graph::PpiGraph g;
    g.get_or_add_node("A");
    g.get_or_add_node("B");
    g.get_or_add_node("C");

    g.add_edge(g.find_node("A").value(), g.find_node("B").value(), 0.5);
    g.add_edge(g.find_node("B").value(), g.find_node("C").value(), 1.5);
    g.add_edge(g.find_node("C").value(), g.find_node("A").value(), 2.0);

    std::vector<double> degrees_weighted =
        tangle::algo::degree_centrality(g, true);

    // Expected weighted degrees:
    // A: 0.5 (to B) + 2.0 (to C) = 2.5
    // B: 0.5 (to A) + 1.5 (to C) = 2.0
    // C: 1.5 (to B) + 2.0 (to A) = 3.5
    REQUIRE(degrees_weighted.size() == 3);
    REQUIRE(degrees_weighted[g.find_node("A").value()] == Approx(2.5));
    REQUIRE(degrees_weighted[g.find_node("B").value()] == Approx(2.0));
    REQUIRE(degrees_weighted[g.find_node("C").value()] == Approx(3.5));
  }
}

TEST_CASE("Louvain Community Detection algorithm", "[algo][community]") {
  // Create a graph with two distinct communities connected by a single edge
  tangle::graph::PpiGraph g;
  // Community 1: nodes 0-3
  for (int i = 0; i <= 3; ++i) {
    g.get_or_add_node(std::to_string(i));
  }
  // Community 2: nodes 4-7
  for (int i = 4; i <= 7; ++i) {
    g.get_or_add_node(std::to_string(i));
  }

  // Add edges within community 1
  g.add_edge(0, 1);
  g.add_edge(0, 2);
  g.add_edge(0, 3);
  g.add_edge(1, 2);
  g.add_edge(1, 3);
  g.add_edge(2, 3);

  // Add edges within community 2
  g.add_edge(4, 5);
  g.add_edge(4, 6);
  g.add_edge(4, 7);
  g.add_edge(5, 6);
  g.add_edge(5, 7);
  g.add_edge(6, 7);

  // Add a single bridge edge between communities
  g.add_edge(0, 4);

  auto communities = tangle::algo::louvain_community(g);

  // The algorithm should find 2 communities
  REQUIRE(communities.size() == 2);

  // Sort the communities and the nodes within them for consistent comparison
  for (auto &community : communities) {
    std::sort(community.begin(), community.end());
  }
  std::sort(communities.begin(), communities.end(),
            [](const auto &a, const auto &b) { return a[0] < b[0]; });

  // Check that the communities are as expected
  std::vector<tangle::NodeId> expected_comm1 = {0, 1, 2, 3};
  std::vector<tangle::NodeId> expected_comm2 = {4, 5, 6, 7};

  // The exact assignment to community 0 or 1 might vary, so we check both
  // possibilities
  bool case1 =
      (communities[0] == expected_comm1 && communities[1] == expected_comm2);
  bool case2 =
      (communities[0] == expected_comm2 && communities[1] == expected_comm1);

  REQUIRE((case1 || case2));
}

TEST_CASE("GO Annotation and Enrichment", "[annotate]") {
  // 1. Load annotations from the dummy GAF file
  tangle::annotate::AnnotationDb db;
  // The dummy file is in tests/, and the test runs from the build/ directory
  db.load_from_gaf("../tests/dummy_goa.gaf");

  SECTION("AnnotationDb loading") {
    REQUIRE(db.get_all_annotated_proteins().size() == 5);
    REQUIRE(db.get_all_go_terms().size() == 3);
    REQUIRE(db.get_annotations("P12345").size() == 2);
    auto terms_p12345 = db.get_annotations("P12345");
    REQUIRE(std::binary_search(terms_p12345.begin(), terms_p12345.end(),
                               "GO:0005575"));
    auto terms_c2c2c2 = db.get_annotations("C2C2C2");
    REQUIRE(std::binary_search(terms_c2c2c2.begin(), terms_c2c2c2.end(),
                               "GO:1234567"));
    REQUIRE(db.get_annotations("NonExistentProtein").empty());
  }

  SECTION("GO Enrichment analysis") {
    // Define a protein set that should be enriched for GO:0005575
    std::vector<tangle::ProteinId> protein_set = {"P12345", "Q99999"};

    // Perform enrichment
    auto results = tangle::annotate::go_enrichment(protein_set, db);

    // N=5 (total proteins), K=3 (with GO:0005575), n=2 (in set), k=2 (in set
    // with term) We are testing for over-representation of GO:0005575

    REQUIRE_FALSE(results.empty());

    bool term_found = false;
    for (const auto &res : results) {
      if (res.go_term == "GO:0005575") {
        term_found = true;
        REQUIRE(res.count_in_set == 2);        // k
        REQUIRE(res.total_in_set == 2);        // n
        REQUIRE(res.count_in_background == 3); // K
        REQUIRE(res.total_in_background == 5); // N

        // P(X>=2) for HGT(N=5, K=3, n=2)
        // P(X=2) = (C(3,2) * C(2,0)) / C(5,2) = (3 * 1) / 10 = 0.3
        // Since k cannot be > n, P(X>=2) is just P(X=2)
        REQUIRE(res.p_value == Approx(0.3));
      }
    }
    REQUIRE(term_found);
  }
}

TEST_CASE("STRING Importer functionality", "[io][string]") {
  // The dummy file is in tests/, and the test runs from the build/ directory
  std::string string_filepath = "../tests/dummy_string.txt";

  SECTION("Default minimum score (700)") {
    tangle::graph::PpiGraph g = tangle::io::load_from_string(string_filepath);
    REQUIRE(g.num_edges() == 3);
    REQUIRE(g.num_nodes() == 4);
  }

  SECTION("Custom minimum score (100)") {
    tangle::graph::PpiGraph g =
        tangle::io::load_from_string(string_filepath, 100.0);
    REQUIRE(g.num_edges() == 4);
    REQUIRE(g.num_nodes() == 5);
  }

  SECTION("Very low minimum score (0)") {
    tangle::graph::PpiGraph g =
        tangle::io::load_from_string(string_filepath, 0.0);
    REQUIRE(g.num_edges() == 5);
    REQUIRE(g.num_nodes() == 5);
  }
}

TEST_CASE("SBML Exporter functionality", "[export][sbml]") {
  // Create a simple graph to export
  tangle::graph::PpiGraph g;
  g.get_or_add_node("PROT_A");
  g.get_or_add_node("PROT_B");
  g.add_edge(g.find_node("PROT_A").value(), g.find_node("PROT_B").value());

  // Define a temporary file path
  std::string sbml_filepath = "test_export.sbml";

  // Export the graph
  tangle::export_::save_to_sbml(g, sbml_filepath);

  // Read the file content back
  std::ifstream infile(sbml_filepath);
  REQUIRE(infile.is_open());
  std::string content((std::istreambuf_iterator<char>(infile)),
                      std::istreambuf_iterator<char>());
  infile.close();

  // Clean up the file
  // std::remove(sbml_filepath.c_str());

  // Perform basic checks on the content
  REQUIRE(content.find("<sbml") != std::string::npos);
  REQUIRE(content.find("</sbml>") != std::string::npos);
  REQUIRE(content.find("<model") != std::string::npos);
  REQUIRE(content.find("<species id=\"PROT_A\"") != std::string::npos);
  REQUIRE(content.find("<species id=\"PROT_B\"") != std::string::npos);
  REQUIRE(content.find("<reaction id=\"r0\"") != std::string::npos);
  REQUIRE(content.find("<speciesReference species=\"PROT_A\"") !=
          std::string::npos);
  REQUIRE(content.find("<speciesReference species=\"PROT_B\"") !=
          std::string::npos);
}

TEST_CASE("Robustness: Edge Cases and Error Handling", "[robustness]") {
  SECTION("Empty file handling") {
    std::string empty_file = create_temp_edgelist_file("", "empty_");
    // Depending on implementation, this might throw or return empty graph.
    // Based on edgelist_io.cpp logic (viewed earlier), it likely returns empty
    // graph.
    tangle::graph::PpiGraph g = tangle::io::load_edgelist(empty_file);
    REQUIRE(g.num_nodes() == 0);
    REQUIRE(g.num_edges() == 0);
    std::remove(empty_file.c_str());
  }

  SECTION("Non-existent file") {
    REQUIRE_THROWS(tangle::io::load_edgelist("non_existent_file_xyz.txt"));
  }

  SECTION("Disconnected Graph Analysis") {
    tangle::graph::PpiGraph g;
    // Component 1: A-B
    g.add_edge(g.get_or_add_node("A"), g.get_or_add_node("B"));
    // Component 2: C-D
    g.add_edge(g.get_or_add_node("C"), g.get_or_add_node("D"));

    // Centrality should still work
    auto degrees = tangle::algo::degree_centrality(g);
    REQUIRE(degrees.size() == 4);
    for (double d : degrees) {
      REQUIRE(d == 1.0);
    }

    // Communities should find at least 2
    auto comms = tangle::algo::louvain_community(g);
    REQUIRE(comms.size() >= 2);
  }
}

TEST_CASE("Performance Benchmarks", "[benchmark]") {
  // Setup a larger graph for benchmarking
  tangle::graph::PpiGraph large_graph;
  int num_nodes = 1000;
  for (int i = 0; i < num_nodes; ++i) {
    large_graph.get_or_add_node(std::to_string(i));
  }
  // Create a ring
  for (int i = 0; i < num_nodes; ++i) {
    large_graph.add_edge(i, (i + 1) % num_nodes);
  }

  BENCHMARK("Degree Centrality (1k nodes, ring)") {
    return tangle::algo::degree_centrality(large_graph);
  };

  BENCHMARK("Louvain Community (1k nodes, ring)") {
    return tangle::algo::louvain_community(large_graph);
  };
}
