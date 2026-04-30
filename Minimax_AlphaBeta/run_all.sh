#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$SCRIPT_DIR/bin"
INPUT_DIR="$SCRIPT_DIR/input"
SRC_FILE="${1:-$SCRIPT_DIR/src/main.cpp}"
OUTPUT_DIR="${2:-$SCRIPT_DIR/output}"
SOLVER="$BIN_DIR/solver"

mkdir -p "$BIN_DIR" "$OUTPUT_DIR"

if [ ! -f "$SRC_FILE" ]; then
    echo "Source file not found: $SRC_FILE" >&2
    exit 1
fi

echo "Compiling solver..."
g++ -std=c++17 -O2 -Wall -Wextra -o "$SOLVER" "$SRC_FILE"

shopt -s nullglob
input_files=("$INPUT_DIR"/input_*.txt)
shopt -u nullglob

if [ "${#input_files[@]}" -eq 0 ]; then
    echo "No input files found in $INPUT_DIR" >&2
    exit 1
fi

for input_file in "${input_files[@]}"; do
    file_name="$(basename "$input_file")"
    index="${file_name#input_}"
    index="${index%.txt}"
    output_file="$OUTPUT_DIR/output_${index}.txt"

    "$SOLVER" "$input_file" "$output_file"
    echo "Generated $(basename "$output_file")"
done

echo "All test files processed."
