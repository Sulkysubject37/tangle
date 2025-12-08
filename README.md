# tangle

```text
       ╔══════════╗
  ┌────╢  ●──●──●  ╟────┐
  │    ║   ╲ │ ╱   ║    │
  │    ║  ●─●●─●   ║    │
  └────╢   ╱ │ ╲   ╟────┘
       ╚══════════╝
```

`tangle` is a high-performance C++ library for **static protein–protein interaction (PPI) network analysis**, designed to support drug discovery and systems biology. It offers a clean C++ API, a powerful CLI, and an interactive TUI.

## 1. Core Library
Built for speed and memory efficiency, the `tangle` library provides:
- **Graph Engine**: Optimized adjacency lists for large scale networks (STRING, BioGRID).
- **Algorithms**:
    - **Centrality**: Degree centrality.
    - **Communities**: Louvain community detection.
- **Enrichment**: Hypergeometric GO enrichment analysis.
    - **Optimized**: 1000x faster than standard implementations via pre-computed frequency maps.
    - **Smart IDs**: Supports both UniProt IDs and Gene Symbols (e.g., "FGF1" matches "P05230").
- **I/O**: robust importers for PPI standards and SBML export.

## 2. CLI (`tangle`)
The command-line interface exposes the library functions for scripting and pipelines.

### Usage
```bash
# Import a STRING network
tangle import --in=9606.protein.links.txt --out=human.edgelist --score=700

# Run analysis (Louvain clustering)
tangle analyze --in=human.edgelist --out=communities.tsv

# Run GO Enrichment
tangle annotate --in-comm=communities.tsv --in-gaf=goa_human.gaf --out=enrichment.tsv

# Export to SBML
tangle export --in=human.edgelist --out=network.sbml
```

## 3. TUI (`tangle-tui`)
An interactive Text User Interface (using FTXUI) for exploring networks without writing code.

### Key Features
- **Tabbed Interface**: Organized workflow: **Import** -> **Analysis** -> **Annotation** -> **Export**.
- **One-Click Enrichment**:
    - Automatically loads GAF annotation files when needed.
    - Instant analysis of communities.
    - **Save Results**: Export your findings to TSV with a single click.
- **Robust & Thread-Safe**:
    - Long-running tasks happen in the background.
    - Loading spinners and real-time logs keep you informed.
- **Interactive**:
    - Preview input files before loading.
    - User-friendly pagination prevents UI lag.

## 4. End-to-End Pipeline Guide
Follow this guide to run a complete analysis pipeline using the provided test data.

### 1. Import Raw Data
Convert the raw STRING-like data (`test_data/test_case.tsv`) into Tangle's optimized edgelist format.
```bash
./tangle import --in=test_data/test_case.tsv --out=graph.edgelist --format=string --score=400 --score-col=12 --delimiter=tab
```

### 2. Community Detection
Identify functional modules (communities) using the Louvain algorithm.
```bash
./tangle analyze --in=graph.edgelist --out=communities.tsv
```

### 3. Functional Enrichment
 Annotate the discovered communities using Gene Ontology (GO) terms.
```bash
# Using the provided dummy GAF for testing (or provide your own GAF file)
./tangle annotate --in-comm=communities.tsv --in-gaf=tests/dummy_goa.gaf --out=enrichment_results.tsv
```

### 4. Export to SBML
Export the network structure to SBML for use in other systems biology tools.
```bash
./tangle export --in=graph.edgelist --out=network.sbml
```

## Getting Started

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./tangle-tui
```

## Citation
MD. Arshad, Department of Computer Science, Jamia Millia Islamia.

## Blog
Link : https://subconc.hashnode.dev/tangle


