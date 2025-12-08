#!/bin/bash
#
# A simple demo script for the tangle CLI.
# This script runs a full analysis pipeline and a benchmark.
#

set -e

# --- Configuration ---
# Path to the tangle executable
TANGLE_EXE=./build/tangle
# Path to test data
STRING_FILE=./tests/dummy_string.txt
GAF_FILE=./tests/dummy_goa.gaf
# Output directory
OUT_DIR=./demo_output

# ---
# 1. Clean up and set up output directory
# ---
echo "--- Setting up output directory ---"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
echo "Output will be in '$OUT_DIR/'"
echo ""

# ---
# 2. Import network from STRING
# ---
echo "--- 1. Importing STRING network ---"
$TANGLE_EXE import --in="$STRING_FILE" --out="$OUT_DIR/network.edgelist" --score=700
echo ""

# ---
# 3. Analyze network to find communities
# ---
echo "--- 2. Analyzing network for communities ---"
$TANGLE_EXE analyze --in="$OUT_DIR/network.edgelist" --out="$OUT_DIR/communities.tsv"
echo ""

# ---
# 4. Annotate communities with GO terms
# ---
echo "--- 3. Annotating communities with GO terms ---"
$TANGLE_EXE annotate --in-comm="$OUT_DIR/communities.tsv" --in-gaf="$GAF_FILE" --out="$OUT_DIR/enrichment.tsv"
echo ""

# ---
# 5. Export network to SBML
# ---
echo "--- 4. Exporting network to SBML ---"
$TANGLE_EXE export --in="$OUT_DIR/network.edgelist" --out="$OUT_DIR/network.sbml"
echo ""

# ---
# 6. Run benchmark
# ---
echo "--- 5. Running benchmark ---"
$TANGLE_EXE analyze --in="$OUT_DIR/network.edgelist" --benchmark
echo ""


echo "--- Demo complete! ---"
echo "Check the following output files:"
echo " - $OUT_DIR/network.edgelist"
echo " - $OUT_DIR/communities.tsv"
echo " - $OUT_DIR/enrichment.tsv"
echo " - $OUT_DIR/network.sbml"