#include <chrono>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/table.hpp"

#include "tangle/algo/community.hpp"
#include "tangle/annotate/annotation_db.hpp"
#include "tangle/annotate/go_enrichment.hpp"
#include "tangle/export/sbml_exporter.hpp"
#include "tangle/graph.hpp"
#include "tangle/io/biogrid_importer.hpp"
#include "tangle/io/edgelist_io.hpp"
#include "tangle/io/string_importer.hpp"

using namespace ftxui;

// ----------------------------------------------------------------------------
// Application state
// ----------------------------------------------------------------------------
struct AppState {
  // --- UI State ---
  std::string edgelist_path;
  std::string string_path;
  std::string biogrid_path;
  std::string gaf_path;
  std::string sbml_path;
  std::string enrichment_out_path = "enrichment_results.tsv";
  std::string min_score_str = "700";
  std::string score_col_str = "10";
  int delimiter_choice = 0;
  std::vector<std::string> delimiter_entries = {"Space", "Tab"};

  // --- Data and Stats (protected by mutex) ---
  // shared_ptr allows background threads to hold a reference to data
  // even if the UI triggers a new load (replacing the pointer in AppState).
  std::mutex mtx;
  bool is_loading = false;
  std::string status_message = "Ready";
  std::shared_ptr<tangle::graph::PpiGraph> graph;
  std::shared_ptr<tangle::annotate::AnnotationDb> annotation_db;
  std::vector<std::vector<tangle::NodeId>> communities;
  std::vector<tangle::annotate::GoEnrichmentResult> enrichment_results;
  int node_count = 0;
  int edge_count = 0;
  int community_count = 0;

  // --- Logging ---
  std::deque<std::string> logs;
  std::string file_preview;

  // --- Methods ---
  void log(std::string message) {
    std::lock_guard<std::mutex> lock(mtx);
    logs.push_front(message);
    if (logs.size() > 10) {
      logs.pop_back();
    }
  }

  void set_status(std::string msg, bool loading) {
    std::lock_guard<std::mutex> lock(mtx);
    status_message = msg;
    is_loading = loading;
  }

  void preview_file(const std::string &filepath) {
    std::lock_guard<std::mutex> lock(mtx);
    file_preview.clear();
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
      file_preview = "Could not open file for preview.";
      return;
    }
    std::string line;
    for (int i = 0; i < 5 && std::getline(infile, line); ++i) {
      file_preview += line + "\n";
    }
    if (infile.eof()) {
      file_preview += "(End of file)\n";
    } else {
      file_preview += "(...)\n";
    }
  }

  // --- Long running tasks (safe versions) ---
  // Helper to run tasks safely
  template <typename Func> void run_task(Func task, ScreenInteractive *screen) {
    std::thread([this, task, screen] {
      try {
        task();
      } catch (...) {
        log("Unknown error in task.");
        set_status("Error", false);
      }
      if (screen)
        screen->Post(Event::Custom);
    }).detach();
  }

  void load_edgelist_graph_impl(std::string path) {
    set_status("Loading Edgelist...", true);
    log("Attempting to load edgelist graph from " + path + "...");
    preview_file(path);
    try {
      auto temp_graph = std::make_shared<tangle::graph::PpiGraph>(
          tangle::io::load_edgelist(path, false, '\t'));
      std::lock_guard<std::mutex> lock(mtx);
      graph = temp_graph;
      node_count = graph->num_nodes();
      edge_count = graph->num_edges();
      status_message = "Edgelist loaded successfully.";
      is_loading = false;
      logs.push_front(
          "Edgelist graph loaded. Nodes: " + std::to_string(node_count) +
          ", Edges: " + std::to_string(edge_count));
    } catch (const std::exception &e) {
      log("Error loading edgelist: " + std::string(e.what()));
      set_status("Error loading edgelist.", false);
      std::lock_guard<std::mutex> lock(mtx);
      graph.reset();
      node_count = 0;
      edge_count = 0;
    }
  }

  void import_from_string_impl(std::string path, std::string min_score_s,
                               std::string score_col_s, int delim_choice) {
    set_status("Importing STRING...", true);
    log("Attempting to import STRING network from " + path + "...");
    preview_file(path);
    try {
      double min_score = std::stod(min_score_s);
      int score_col = std::stoi(score_col_s);
      char delimiter = (delim_choice == 0) ? ' ' : '\t';
      auto temp_graph = std::make_shared<tangle::graph::PpiGraph>(
          tangle::io::load_from_string(path, min_score, score_col, delimiter));
      std::lock_guard<std::mutex> lock(mtx);
      graph = temp_graph;
      node_count = graph->num_nodes();
      edge_count = graph->num_edges();
      status_message = "STRING Imported successfully.";
      is_loading = false;
      logs.push_front(
          "STRING network imported. Nodes: " + std::to_string(node_count) +
          ", Edges: " + std::to_string(edge_count));
    } catch (const std::exception &e) {
      log("Error importing STRING: " + std::string(e.what()));
      set_status("Error importing STRING.", false);
      std::lock_guard<std::mutex> lock(mtx);
      graph.reset();
      node_count = 0;
      edge_count = 0;
    }
  }

  void import_from_biogrid_impl(std::string path) {
    set_status("Importing BioGRID...", true);
    log("Attempting to import BioGRID network from " + path + "...");
    preview_file(path);
    try {
      auto temp_graph = std::make_shared<tangle::graph::PpiGraph>(
          tangle::io::load_from_biogrid(path));
      std::lock_guard<std::mutex> lock(mtx);
      graph = temp_graph;
      node_count = graph->num_nodes();
      edge_count = graph->num_edges();
      status_message = "BioGRID Imported successfully.";
      is_loading = false;
      logs.push_front(
          "BioGRID network imported. Nodes: " + std::to_string(node_count) +
          ", Edges: " + std::to_string(edge_count));
    } catch (const std::exception &e) {
      log("Error importing BioGRID: " + std::string(e.what()));
      set_status("Error importing BioGRID.", false);
      std::lock_guard<std::mutex> lock(mtx);
      graph.reset();
      node_count = 0;
      edge_count = 0;
    }
  }

  void run_louvain_impl() {
    set_status("Running Louvain...", true);
    log("Running Louvain community detection...");

    std::shared_ptr<tangle::graph::PpiGraph> working_graph;
    {
      std::lock_guard<std::mutex> lock(mtx);
      working_graph = graph;
    }

    if (!working_graph) {
      log("No graph loaded.");
      set_status("No graph loaded.", false);
      return;
    }
    try {
      auto temp_communities = tangle::algo::louvain_community(*working_graph);
      std::lock_guard<std::mutex> lock(mtx);
      if (graph != working_graph) {
        log("Graph changed during analysis. Discarding results.");
        return;
      }
      communities = std::move(temp_communities);
      community_count = communities.size();
      status_message = "Louvain complete.";
      is_loading = false;
      logs.push_front("Louvain complete. Found " +
                      std::to_string(community_count) + " communities.");
    } catch (const std::exception &e) {
      log("Error running Louvain: " + std::string(e.what()));
      set_status("Error running Louvain.", false);
    }
  }

  void load_annotations_impl(std::string path) {
    set_status("Loading GAF...", true);
    log("Attempting to load annotations from " + path + "...");
    preview_file(path);
    try {
      auto temp_db = std::make_shared<tangle::annotate::AnnotationDb>();
      temp_db->load_from_gaf(path);
      std::lock_guard<std::mutex> lock(mtx);
      annotation_db = temp_db;
      status_message = "GAF Loaded.";
      is_loading = false;
      logs.push_front("Annotations loaded. Found " +
                      std::to_string(annotation_db->get_all_go_terms().size()) +
                      " GO terms.");
    } catch (const std::exception &e) {
      log("Error loading GAF file: " + std::string(e.what()));
      set_status("Error loading GAF.", false);
      std::lock_guard<std::mutex> lock(mtx);
      annotation_db.reset();
    }
  }

  void run_go_enrichment_impl() {
    set_status("Running Enrichment...", true);
    log("Running GO enrichment...");

    std::shared_ptr<tangle::graph::PpiGraph> working_graph;
    std::shared_ptr<tangle::annotate::AnnotationDb> working_db;
    std::vector<std::vector<tangle::NodeId>> working_communities;

    {
      std::lock_guard<std::mutex> lock(mtx);
      working_graph = graph;
      working_db = annotation_db;
      working_communities = communities;
    }

    if (working_communities.empty()) {
      set_status("No communities found.", false);
      log("No communities found. Run Louvain first.");
      return;
    }
    if (!working_db) {
      set_status("GAF not loaded.", false);
      log("Annotation database not loaded. Load GAF file first.");
      return;
    }
    try {
      std::vector<tangle::annotate::GoEnrichmentResult> temp_results;
      for (const auto &comm : working_communities) {
        std::vector<tangle::ProteinId> protein_set;
        for (const auto &node_id : comm) {
          protein_set.push_back(working_graph->node(node_id).protein_id);
        }

        // --- ID Mismatch Debugging ---
        int matched_count = 0;
        for (const auto &pid : protein_set) {
          if (working_db->has_annotations(pid))
            matched_count++;
        }

        {
          std::lock_guard<std::mutex> lock(mtx);
          logs.push_front(
              "Community size: " + std::to_string(protein_set.size()) +
              ", Annotated overlap: " + std::to_string(matched_count));
          if (matched_count == 0) {
            logs.push_front("WARNING: No overlap! Check Protein IDs (e.g. '" +
                            (protein_set.empty() ? "?" : protein_set[0]) +
                            "') vs GAF.");
          }
        }
        // -----------------------------

        auto results =
            tangle::annotate::go_enrichment(protein_set, *working_db);
        for (const auto &res : results) {
          if (res.adjusted_p_value <= 1.0) {
            temp_results.push_back(res);
          }
        }
      }
      std::lock_guard<std::mutex> lock(mtx);
      enrichment_results = std::move(temp_results);
      status_message = "Enrichment complete.";
      is_loading = false;
      logs.push_front("GO enrichment complete. Found " +
                      std::to_string(enrichment_results.size()) + " terms.");
    } catch (const std::exception &e) {
      set_status("Error in enrichment.", false);
      log("Error running GO enrichment: " + std::string(e.what()));
    }
  }

  void load_and_run_enrichment_impl(std::string path) {
    load_annotations_impl(path);
    // Check if load was successful
    bool db_loaded = false;
    {
      std::lock_guard<std::mutex> lock(mtx);
      db_loaded = (annotation_db != nullptr);
    }
    if (db_loaded) {
      run_go_enrichment_impl();
    }
  }

  void save_enrichment_results_impl(std::string path) {
    set_status("Saving Results...", true);
    log("Saving enrichment results to " + path + "...");

    std::vector<tangle::annotate::GoEnrichmentResult> results_copy;
    {
      std::lock_guard<std::mutex> lock(mtx);
      results_copy = enrichment_results;
    }

    if (results_copy.empty()) {
      log("No results to save.");
      set_status("No results.", false);
      return;
    }

    std::ofstream outfile(path);
    if (!outfile.is_open()) {
      log("Error: Could not open file " + path);
      set_status("Error saving.", false);
      return;
    }

    outfile
        << "GO Term\tP-Value\tAdj P-Value\tCount in Set\tBackground Check\n";
    for (const auto &res : results_copy) {
      outfile << res.go_term << "\t" << res.p_value << "\t"
              << res.adjusted_p_value << "\t" << res.count_in_set << "\t"
              << res.count_in_background << "\n";
    }

    set_status("Results Saved.", false);
    log("Saved " + std::to_string(results_copy.size()) + " rows to " + path);
  }

  void export_to_sbml_impl(std::string path) {
    set_status("Exporting SBML...", true);
    log("Attempting to export to SBML...");

    std::shared_ptr<tangle::graph::PpiGraph> working_graph;
    {
      std::lock_guard<std::mutex> lock(mtx);
      working_graph = graph;
    }

    if (!working_graph) {
      set_status("No graph loaded.", false);
      log("No graph loaded.");
      return;
    }
    if (path.empty()) {
      path = "output.sbml";
    }
    try {
      tangle::export_::save_to_sbml(*working_graph, path);
      status_message = "Export Successful.";
      is_loading = false;
      logs.push_front("Exported to SBML successfully to " + path + ".");
    } catch (const std::exception &e) {
      status_message = "Export Failed.";
      is_loading = false;
      logs.push_front("Error exporting to SBML: " + std::string(e.what()));
    }
  }
};

// ----------------------------------------------------------------------------
// Main TUI component
// ----------------------------------------------------------------------------
int main(int argc, const char *argv[]) {
  auto state = std::make_shared<AppState>();
  auto screen = ScreenInteractive::TerminalOutput();

  // --- Components ---
  auto input_edgelist = Input(&state->edgelist_path, "edgelist.tsv");
  auto input_string = Input(&state->string_path, "string_links.txt");
  auto input_biogrid = Input(&state->biogrid_path, "biogrid_interactions.tsv");
  auto input_min_score = Input(&state->min_score_str, "700");
  auto input_score_col = Input(&state->score_col_str, "10");
  auto input_gaf = Input(&state->gaf_path, "goa.gaf");
  auto input_sbml = Input(&state->sbml_path, "output.sbml");
  auto delimiter_radiobox =
      Radiobox(&state->delimiter_entries, &state->delimiter_choice);

  auto button_load_edgelist = Button("Load Edgelist", [&] {
    std::string path = state->edgelist_path;
    state->run_task([state, path] { state->load_edgelist_graph_impl(path); },
                    &screen);
  });
  auto button_import_string = Button("Import STRING", [&] {
    std::string path = state->string_path;
    std::string min_s = state->min_score_str;
    std::string col_s = state->score_col_str;
    int delim = state->delimiter_choice;
    state->run_task(
        [state, path, min_s, col_s, delim] {
          state->import_from_string_impl(path, min_s, col_s, delim);
        },
        &screen);
  });
  auto button_import_biogrid = Button("Import BioGRID", [&] {
    std::string path = state->biogrid_path;
    state->run_task([state, path] { state->import_from_biogrid_impl(path); },
                    &screen);
  });
  auto button_louvain = Button("Run Louvain", [&] {
    state->run_task([state] { state->run_louvain_impl(); }, &screen);
  });
  auto button_load_gaf = Button("Load GAF", [&] {
    std::string path = state->gaf_path;
    state->run_task([state, path] { state->load_annotations_impl(path); },
                    &screen);
  });
  auto button_run_enrichment = Button("Run GO Enrichment", [&] {
    bool needs_load = false;
    std::string path = state->gaf_path;
    {
      std::lock_guard<std::mutex> lock(state->mtx);
      if (!state->annotation_db) {
        needs_load = true;
      }
    }

    if (needs_load) {
      if (path.empty()) {
        // Let the run_impl handle the missing DB error message
        state->run_task([state] { state->run_go_enrichment_impl(); }, &screen);
      } else {
        state->run_task(
            [state, path] { state->load_and_run_enrichment_impl(path); },
            &screen);
      }
    } else {
      state->run_task([state] { state->run_go_enrichment_impl(); }, &screen);
    }
  });

  auto input_enrichment_out =
      Input(&state->enrichment_out_path, "enrichment_results.tsv");
  auto button_save_enrichment = Button("Save Results", [&] {
    std::string path = state->enrichment_out_path;
    state->run_task(
        [state, path] { state->save_enrichment_results_impl(path); }, &screen);
  });

  auto button_export_sbml = Button("Export SBML", [&] {
    std::string path = state->sbml_path;
    state->run_task([state, path] { state->export_to_sbml_impl(path); },
                    &screen);
  });
  auto button_quit = Button("Quit", [&] { screen.Exit(); });

  // --- Layout ---
  int tab_index = 0;
  std::vector<std::string> tab_entries = {"Import", "Analysis", "Annotation",
                                          "Export"};
  auto tab_toggle = Toggle(&tab_entries, &tab_index);

  auto tab_container = Container::Tab(
      {
          Container::Vertical({
              input_edgelist,
              button_load_edgelist,
              input_string,
              input_min_score,
              input_score_col,
              delimiter_radiobox,
              button_import_string,
              input_biogrid,
              button_import_biogrid,
          }),
          Container::Vertical({button_louvain}),
          Container::Vertical({input_gaf, button_load_gaf,
                               button_run_enrichment, input_enrichment_out,
                               button_save_enrichment}),
          Container::Vertical({input_sbml, button_export_sbml}),
      },
      &tab_index);

  auto main_container =
      Container::Vertical({tab_toggle, tab_container, button_quit});

  auto renderer = Renderer(main_container, [&] {
    // We do NOT lock the mutex for the entire render, only for checking
    // stats/logs. FTXUI runs in the main thread anyway.

    std::lock_guard<std::mutex> lock(state->mtx);

    bool loading = state->is_loading;
    std::string status = state->status_message;

    auto stats_panel = vbox({
        text("--- Stats ---") | bold,
        text("Nodes: " + std::to_string(state->node_count)),
        text("Edges: " + std::to_string(state->edge_count)),
        text("Communities: " + std::to_string(state->community_count)),
    });

    auto file_preview_panel = vbox({
        text("--- File Preview ---") | bold,
        text(state->file_preview) | yframe | flex,
    });

    std::vector<std::vector<std::string>> enrichment_table_data;
    if (!state->enrichment_results.empty()) {
      enrichment_table_data.push_back({"GO Term", "Adj. P-Value", "Count"});
      int displayed = 0;
      for (const auto &res : state->enrichment_results) {
        if (++displayed > 10)
          break;
        enrichment_table_data.push_back({res.go_term,
                                         std::to_string(res.adjusted_p_value),
                                         std::to_string(res.count_in_set)});
      }
    }
    auto enrichment_table = Table(enrichment_table_data);
    enrichment_table.SelectAll().Border(LIGHT);

    auto results_panel =
        vbox({
            text("--- GO Enrichment Results ---") | bold,
            state->enrichment_results.empty()
                ? text(" (Run enrichment to see results)") | dim
                : enrichment_table.Render(),
        }) |
        yframe;

    Elements log_elements;
    for (const auto &log_msg : state->logs) {
      log_elements.push_back(text(log_msg));
    }
    auto log_panel =
        vbox({text("--- Log ---") | bold, vbox(log_elements) | yframe | flex});

    Element layout;
    if (tab_index == 0) { // Import
      layout = vbox({
          hbox(text("Edgelist: "), input_edgelist->Render() | border | flex),
          button_load_edgelist->Render(),
          separator(),
          hbox(text("STRING: "), input_string->Render() | border | flex),
          hbox(text("Min Score: "), input_min_score->Render() | border),
          hbox(text("Score Col: "), input_score_col->Render() | border),
          hbox(text("Delimiter: "), delimiter_radiobox->Render()),
          button_import_string->Render(),
          separator(),
          hbox(text("BioGRID: "), input_biogrid->Render() | border | flex),
          button_import_biogrid->Render(),
      });
    } else if (tab_index == 1) { // Analysis
      layout = vbox({button_louvain->Render()});
    } else if (tab_index == 2) { // Annotation
      layout = vbox({
          hbox(text("GAF Path: "), input_gaf->Render() | border | flex),
          button_load_gaf->Render(),
          button_run_enrichment->Render(),
      });
    } else { // Export
      layout = vbox({
          hbox(text("SBML Path: "), input_sbml->Render() | border | flex),
          button_export_sbml->Render(),
      });
    }

    auto status_bar = hbox({
                          text(" Status: ") | bold,
                          text(status) | (loading ? color(Color::Yellow)
                                                  : color(Color::Green)),
                          filler(),
                          loading ? spinner(21, 0) : text(""),
                      }) |
                      border;

    auto logo =
        vbox({
            text("       ╔══════════╗") | bold | color(Color::Cyan),
            text("  ┌────╢  ●──●──●  ╟────┐") | bold | color(Color::Cyan),
            text("  │    ║   ╲ │ ╱   ║    │") | bold | color(Color::Cyan),
            text("  │    ║  ●─●●─●   ║    │") | bold | color(Color::Cyan),
            text("  └────╢   ╱ │ ╲   ╟────┘") | bold | color(Color::Cyan),
            text("       ╚══════════╝") | bold | color(Color::Cyan),
            text("           tangle-tui") | bold | color(Color::White),
            text("    static PPI analysis terminal") | dim |
                color(Color::GrayLight),
        }) |
        hcenter;

    return vbox({
               logo,
               separator(),
               tab_toggle->Render(),
               separator(),
               layout | flex,
               separator(),
               hbox({stats_panel | flex, separator(),
                     file_preview_panel | flex}),
               separator(),
               results_panel | flex,
               separator(),
               log_panel | flex,
               separator(),
               status_bar,
               hbox(button_quit->Render()),
           }) |
           border;
  });

  screen.Loop(renderer);

  return 0;
}
