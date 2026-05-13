// 对比加入走子排序（中间列优先）前后 alpha-beta 展开节点数的变化。
//./move_ordering <input_file>
//   best_move
//   alpha-beta（无排序）展开节点数
//   alpha-beta（中间列优先排序）展开节点数

#include "connect_four.h"

// 无排序的 alpha-beta
int abValue(const Board& board, int alpha, int beta, long long& nodes) {
    ++nodes;
    const int tv = evaluateTerminal(board);
    if (tv != kOngoing) return tv;

    const std::vector<int> moves = board.getLegalMoves();//顺序是从左到右
    if (board.current_player == 'X') {// X 作为先手，寻找最大值
        int v = std::numeric_limits<int>::min();
        for (int col : moves) {
            v = std::max(v, abValue(board.applyMove(col), alpha, beta, nodes));
            if (v >= beta) break;
            alpha = std::max(alpha, v);
        }
        return v;
    } else {
        int v = std::numeric_limits<int>::max();
        for (int col : moves) {
            v = std::min(v, abValue(board.applyMove(col), alpha, beta, nodes));
            if (v <= alpha) break;
            beta = std::min(beta, v);
        }
        return v;
    }
}

// 中心列优先排序的 alpha-beta
int abValueOrdered(const Board& board, int alpha, int beta, long long& nodes) {
    ++nodes;
    const int tv = evaluateTerminal(board);
    if (tv != kOngoing) return tv;

    const std::vector<int> moves = board.getLegalMovesOrdered();// 顺序是按照 kMoveOrder 从左到右
    if (board.current_player == 'X') {// X 作为先手，寻找最大值
        int v = std::numeric_limits<int>::min();
        for (int col : moves) {
            v = std::max(v, abValueOrdered(board.applyMove(col), alpha, beta, nodes));
            if (v >= beta) break;
            alpha = std::max(alpha, v);
        }
        return v;
    } else {
        int v = std::numeric_limits<int>::max();
        for (int col : moves) {
            v = std::min(v, abValueOrdered(board.applyMove(col), alpha, beta, nodes));
            if (v <= alpha) break;
            beta = std::min(beta, v);
        }
        return v;
    }
}

struct Result {
    int best_move = -1;
    int value = kDraw;
    long long nodes = 0;
};


template <typename ValueFn>
Result solve(const Board& board, ValueFn fn, bool use_ordering) {
    Result res;
    const std::vector<int> moves = use_ordering
        ? board.getLegalMovesOrdered()
        : board.getLegalMoves();

    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();

    if (board.current_player == 'X') {
        res.value = std::numeric_limits<int>::min();
        for (int col : moves) {
            int v = fn(board.applyMove(col), alpha, beta, res.nodes);
            if (v > res.value) { res.value = v; res.best_move = col; }
            alpha = std::max(alpha, res.value);
        }
    } else {
        res.value = std::numeric_limits<int>::max();
        for (int col : moves) {
            int v = fn(board.applyMove(col), alpha, beta, res.nodes);
            if (v < res.value) { res.value = v; res.best_move = col; }
            beta = std::min(beta, res.value);
        }
    }
    return res;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    Board board;
    if (!board.loadFromFile(argv[1])) {
        std::cerr << "Failed to read: " << argv[1] << '\n';
        return 1;
    }

    Result plain   = solve(board, abValue,        false);
    Result ordered = solve(board, abValueOrdered, true);

    std::cout << "Best move (plain  alpha-beta): " << plain.best_move   << '\n';
    std::cout << "Best move (ordered alpha-beta): " << ordered.best_move << '\n';
    std::cout << "alpha-beta (no ordering) nodes : " << plain.nodes   << '\n';
    std::cout << "alpha-beta (center-first) nodes: " << ordered.nodes << '\n';
    double ratio = plain.nodes > 0
        ? 100.0 * (plain.nodes - ordered.nodes) / plain.nodes : 0.0;
    std::cout << "Node reduction: " << ratio << "%\n";
    return 0;
}
