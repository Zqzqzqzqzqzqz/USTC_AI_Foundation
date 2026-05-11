#!/usr/bin/env bash
# run_advanced.sh — 编译并运行三个提高内容程序
# 用法：bash run_advanced.sh [web|play|ordering|depth]
#   web    : 编译 web，启动 Web 服务（默认端口 8080）
#   play     : 启动命令行人机对弈（可加 O 参数让 AI 先手）
#   ordering : 对所有样例比较有序/无序 alpha-beta 节点数
#   depth    : 对所有样例运行深度限制搜索（默认 depth=8）
# 不带参数时依次执行 ordering 和 depth。

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$SCRIPT_DIR/src"
BIN="$SCRIPT_DIR/bin"
INPUT="$SCRIPT_DIR/input"

mkdir -p "$BIN"

CXX_FLAGS="-std=c++17 -O2 -Wall -Wextra"

compile_if_needed() {
    local src="$1" bin="$2"
    if [ ! -f "$bin" ] || [ "$src" -nt "$bin" ] || [ "$SRC/game.h" -nt "$bin" ]; then
        echo "Compiling $(basename "$src")..."
        g++ $CXX_FLAGS -I"$SRC" -o "$bin" "$src"
    fi
}

MODE="${1:-all}"
shift || true   # remaining args forwarded to play

case "$MODE" in
web)
    compile_if_needed "$SRC/web.cpp" "$BIN/web"
    PORT="${1:-8080}"
    echo "Starting web server on http://localhost:$PORT"
    python3 "$SCRIPT_DIR/web.py" "$PORT"
    ;;

play)
    compile_if_needed "$SRC/play.cpp" "$BIN/play"
    "$BIN/play" "$@"
    ;;

ordering)
    compile_if_needed "$SRC/move_ordering.cpp" "$BIN/move_ordering"
    echo "=== Move ordering comparison ==="
    for f in "$INPUT"/input_*.txt; do
        echo "--- $(basename "$f") ---"
        "$BIN/move_ordering" "$f"
    done
    ;;

depth)
    compile_if_needed "$SRC/depth_search.cpp" "$BIN/depth_search"
    DEPTH="${1:-8}"
    echo "=== Depth-limited search (depth=$DEPTH) ==="
    for f in "$INPUT"/input_*.txt; do
        echo "--- $(basename "$f") ---"
        "$BIN/depth_search" "$f" "$DEPTH"
    done
    ;;

all)
    compile_if_needed "$SRC/move_ordering.cpp" "$BIN/move_ordering"
    compile_if_needed "$SRC/depth_search.cpp"  "$BIN/depth_search"
    echo "=== Move ordering comparison ==="
    for f in "$INPUT"/input_*.txt; do
        echo "--- $(basename "$f") ---"
        "$BIN/move_ordering" "$f"
    done
    echo ""
    echo "=== Depth-limited search (depth=8) ==="
    for f in "$INPUT"/input_*.txt; do
        echo "--- $(basename "$f") ---"
        "$BIN/depth_search" "$f" 8
    done
    ;;

*)
    echo "Usage: bash run_advanced.sh [serve [port]|play [O]|ordering|depth [n]|all]" >&2
    exit 1
    ;;
esac
