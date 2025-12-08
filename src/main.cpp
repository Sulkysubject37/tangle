#include "tangle/algo/centrality.hpp"
#include "tangle/algo/community.hpp"
#include "tangle/annotate/annotation_db.hpp"
#include "tangle/annotate/go_enrichment.hpp"
#include "tangle/export/sbml_exporter.hpp"
#include "tangle/graph.hpp"
#include "tangle/io/biogrid_importer.hpp"
#include "tangle/io/edgelist_io.hpp"
#include "tangle/io/string_importer.hpp"
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if TANGLE_WITH_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

// ----------------------------------------------------------------------------
// Logging
// ----------------------------------------------------------------------------
int verbosity_level = 1; // 0=quiet, 1=normal, 2=verbose

void log(int level, const std::string &message) {
  if (verbosity_level >= level) {
    std::cout << message;
  }
}

void log_error(const std::string &message) {
  if (verbosity_level > 0) {
    std::cerr << message;
  }
}

// ----------------------------------------------------------------------------
// CLI subcommand handlers
// ----------------------------------------------------------------------------

void handle_import(const std::map<std::string, std::string> &args) {
  if (args.find("in") == args.end() || args.find("out") == args.end()) {
    log_error("Usage: tangle import --in=<filepath> --out=<filepath> "
              "[--format=string|biogrid] [--score=<min_score>]\n");
    return;
  }

  const std::string &infile = args.at("in");
  const std::string &outfile = args.at("out");
  std::string format = "string";
  if (args.count("format")) {
    format = args.at("format");
  }

  tangle::graph::PpiGraph graph;
  if (format == "string") {
    double min_score = 700.0;
    if (args.count("score")) {
      min_score = std::stod(args.at("score"));
    }
    int score_col = 10;
    if (args.count("score-col")) {
      score_col = std::stoi(args.at("score-col"));
    }
    char delimiter = ' ';
    if (args.count("delimiter")) {
      std::string d = args.at("delimiter");
      if (d == "tab" || d == "\\t")
        delimiter = '\t';
      else if (!d.empty())
        delimiter = d[0];
    }

    log(1, "Importing from STRING file '" + infile + "' with min score " +
               std::to_string(min_score) + ", score column " +
               std::to_string(score_col) + ", delimiter '" +
               (delimiter == '\t' ? "\\t" : std::string(1, delimiter)) +
               "'...\n");
    graph =
        tangle::io::load_from_string(infile, min_score, score_col, delimiter);
  } else if (format == "biogrid") {
    log(1, "Importing from BioGRID file '" + infile + "'...\n");
    graph = tangle::io::load_from_biogrid(infile);
  } else {
    log_error("Error: Unknown format '" + format +
              "'. Supported: string, biogrid\n");
    return;
  }

  log(1, "  -> Imported " + std::to_string(graph.num_nodes()) + " nodes and " +
             std::to_string(graph.num_edges()) + " edges.\n");

  log(1, "Saving graph to '" + outfile + "'...\n");
  tangle::io::save_edgelist(graph, outfile);
  log(1, "  -> Done.\n");
}

void handle_analyze(const std::map<std::string, std::string> &args) {
  if (args.find("in") == args.end()) {
    log_error("Usage: tangle analyze --in=<edgelist_path> "
              "[--out=<communities_path>] [--format=tsv|json] [--benchmark]\n");
    return;
  }

  bool benchmark = args.count("benchmark");

  if (!benchmark && args.find("out") == args.end()) {
    log_error("Usage: tangle analyze --in=<edgelist_path> "
              "--out=<communities_path> [--format=tsv|json]\n");
    return;
  }

  const std::string &infile = args.at("in");

  log(1, "Loading graph from '" + infile + "'...\n");
  tangle::graph::PpiGraph graph =
      tangle::io::load_edgelist(infile, false); // Assuming unweighted for now
  log(1, "  -> Loaded " + std::to_string(graph.num_nodes()) + " nodes and " +
             std::to_string(graph.num_edges()) + " edges.\n");

  if (benchmark) {
    log(1, "--- Running Benchmarks ---\n");

    auto start_louvain = std::chrono::high_resolution_clock::now();
    auto communities = tangle::algo::louvain_community(graph);
    auto end_louvain = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> louvain_ms =
        end_louvain - start_louvain;
    log(1, "Louvain community detection: " +
               std::to_string(louvain_ms.count()) + " ms\n");

    auto start_degree = std::chrono::high_resolution_clock::now();
    auto degrees = tangle::algo::degree_centrality(graph);
    auto end_degree = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> degree_ms =
        end_degree - start_degree;
    log(1, "Degree centrality: " + std::to_string(degree_ms.count()) + " ms\n");

  } else { // tsv
    const std::string &outfile = args.at("out");
    std::string format = "tsv";
    if (args.count("format")) {
      format = args.at("format");
    }

    log(1, "Running Louvain community detection...\n");
    auto communities = tangle::algo::louvain_community(graph);
    log(1,
        "  -> Found " + std::to_string(communities.size()) + " communities.\n");

    log(1, "Saving communities to '" + outfile + "' in " + format +
               " format...\n");
    std::ofstream out_fs(outfile);

    if (format == "json") {
#if TANGLE_WITH_JSON
      json j;
      for (const auto &community : communities) {
        std::vector<std::string> protein_ids;
        for (const auto &node_id : community) {
          protein_ids.push_back(graph.node(node_id).protein_id);
        }
        j.push_back(protein_ids);
      }
      out_fs << j.dump(2); // Indent with 2 spaces
#else
      log_error("Error: JSON format is not available. Please compile with "
                "-DTANGLE_WITH_JSON=ON\n");
      return;
#endif
    } else { // tsv
      for (const auto &community : communities) {
        for (size_t i = 0; i < community.size(); ++i) {
          out_fs << graph.node(community[i]).protein_id
                 << (i == community.size() - 1 ? "" : "\t");
        }
        out_fs << "\n";
      }
    }
    log(1, "  -> Done.\n");
  }
}

void handle_annotate(const std::map<std::string, std::string> &args) {
  if (args.find("in-comm") == args.end() || args.find("in-gaf") == args.end() ||
      args.find("out") == args.end()) {
    log_error("Usage: tangle annotate --in-comm=<communities_path> "
              "--in-gaf=<gaf_path> --out=<results_path> [--format=tsv|json]\n");
    return;
  }

  const std::string &comm_file = args.at("in-comm");
  const std::string &gaf_file = args.at("in-gaf");
  const std::string &outfile = args.at("out");
  std::string format = "tsv";
  if (args.count("format")) {
    format = args.at("format");
  }
  double p_cutoff = 0.05;
  if (args.count("p-cutoff")) {
    p_cutoff = std::stod(args.at("p-cutoff"));
  }

  log(1, "Loading annotations from '" + gaf_file + "'...\n");
  tangle::annotate::AnnotationDb db;
  db.load_from_gaf(gaf_file);
  log(1, "  -> Loaded " + std::to_string(db.get_all_go_terms().size()) +
             " GO terms for " +
             std::to_string(db.get_all_annotated_proteins().size()) +
             " proteins.\n");

  log(1, "Loading communities from '" + comm_file + "'...\n");
  std::ifstream comm_fs(comm_file);
  std::string line;
  int community_idx = 0;

  std::ofstream out_fs(outfile);
#if TANGLE_WITH_JSON
  json all_results = json::array();
#endif

  if (format == "tsv") {
    out_fs << "community\tgo_term\tp_value\tadj_p_value\tcount_in_set\ttotal_"
              "in_set\tcount_in_bg\ttotal_in_bg\n";
  }

  while (std::getline(comm_fs, line)) {
    std::vector<tangle::ProteinId> community;
    std::stringstream ss(line);
    std::string protein_id;
    while (std::getline(ss, protein_id, '\t')) {
      community.push_back(protein_id);
    }

    if (community.empty())
      continue;

    log(2, "  -> Analyzing community " + std::to_string(community_idx) +
               " with " + std::to_string(community.size()) + " proteins...\n");
    auto results = tangle::annotate::go_enrichment(community, db);

    for (const auto &res : results) {
      if (res.adjusted_p_value <= p_cutoff) {
        if (format == "json") {
#if TANGLE_WITH_JSON
          json j_res;
          j_res["community"] = community_idx;
          j_res["go_term"] = res.go_term;
          j_res["p_value"] = res.p_value;
          j_res["adj_p_value"] = res.adjusted_p_value;
          j_res["count_in_set"] = res.count_in_set;
          j_res["total_in_set"] = res.total_in_set;
          j_res["count_in_background"] = res.count_in_background;
          j_res["total_in_background"] = res.total_in_background;
          all_results.push_back(j_res);
#endif
        } else { // tsv
          out_fs << community_idx << "\t" << res.go_term << "\t" << res.p_value
                 << "\t" << res.adjusted_p_value << "\t" << res.count_in_set
                 << "\t" << res.total_in_set << "\t" << res.count_in_background
                 << "\t" << res.total_in_background << "\n";
        }
      }
    }
    community_idx++;
  }

#if TANGLE_WITH_JSON
  if (format == "json") {
    out_fs << all_results.dump(2);
  }
#else
  if (format == "json") {
    log_error("Error: JSON format is not available. Please compile with "
              "-DTANGLE_WITH_JSON=ON\n");
  }
#endif

  log(1, "Annotation results saved to '" + outfile + "'.\n");
}

void handle_export(const std::map<std::string, std::string> &args) {
  if (args.find("in") == args.end() || args.find("out") == args.end()) {
    log_error("Usage: tangle export --in=<edgelist_path> --out=<sbml_path>\n");
    return;
  }

  const std::string &infile = args.at("in");
  const std::string &outfile = args.at("out");

  log(1, "Loading graph from '" + infile + "'...\n");
  tangle::graph::PpiGraph graph = tangle::io::load_edgelist(infile, false);
  log(1, "  -> Loaded " + std::to_string(graph.num_nodes()) + " nodes and " +
             std::to_string(graph.num_edges()) + " edges.\n");

  log(1, "Exporting graph to SBML at '" + outfile + "'...\n");
  tangle::export_::save_to_sbml(graph, outfile);
  log(1, "  -> Done.\n");
}

// ----------------------------------------------------------------------------
// Simple command-line parser
// ----------------------------------------------------------------------------

void print_usage() {
  log(1, "Usage: tangle <subcommand> [options]\n\n");
  log(1, "Subcommands:\n");
  log(1, "  import    Import a PPI network (e.g., from STRING)\n");
  log(1, "            --in=<filepath> --out=<edgelist_path> "
         "[--score=<min_score>]\n");
  log(1, "  analyze   Run network analysis algorithms\n");
  log(1, "            --in=<edgelist_path> --out=<communities_path> "
         "[--format=tsv|json] [--benchmark]\n");
  log(1, "  annotate  Perform functional enrichment\n");
  log(1, "            --in-comm=<communities_path> --in-gaf=<gaf_path> "
         "--out=<results_path> [--format=tsv|json] [--p-cutoff=<p_value>]\n");
  log(1, "  export    Export a network to a file\n");
  log(1, "            --in=<edgelist_path> --out=<sbml_path>\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  std::string subcommand = "";
  std::map<std::string, std::string> args;
  std::vector<std::string> positional_args;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--", 0) == 0) {
      if (arg == "--verbose") {
        verbosity_level = 2;
        continue;
      }
      if (arg == "--quiet") {
        verbosity_level = 0;
        continue;
      }
      if (arg == "--benchmark") {
        args["benchmark"] = "true";
        continue;
      }

      size_t eq_pos = arg.find('=');
      if (eq_pos != std::string::npos) {
        args[arg.substr(2, eq_pos - 2)] = arg.substr(eq_pos + 1);
      }
    } else {
      positional_args.push_back(arg);
    }
  }

  if (positional_args.empty()) {
    log_error("Error: No subcommand provided.\n");
    print_usage();
    return 1;
  }
  subcommand = positional_args[0];

  std::map<std::string,
           std::function<void(const std::map<std::string, std::string> &)>>
      handlers = {{"import", handle_import},
                  {"analyze", handle_analyze},
                  {"annotate", handle_annotate},
                  {"export", handle_export}};

  if (handlers.find(subcommand) == handlers.end()) {
    log_error("Error: Unknown subcommand '" + subcommand + "'\n");
    print_usage();
    return 1;
  }

  // Call the handler
  handlers[subcommand](args);

  return 0;
}
