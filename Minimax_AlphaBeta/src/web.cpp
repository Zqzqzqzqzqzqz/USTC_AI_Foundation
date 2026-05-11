// ai_move.cpp
// 供 Web 服务端调用的 AI 求解器。
// 从 stdin 读取棋盘状态，将最优落子列号输出到 stdout，无其他输出。
//
// stdin 格式（与实验输入文件相同）：
//   第1行：当前行动方 'X' 或 'O'
//   后6行：每行7字符的棋盘

#include "connect_four.h"

constexpr int kWinScore = 1000000;
constexpr int kAiDepth  = 10;

int abSearch(const Board& board, int alpha, int beta, int depth, long long& nodes) {
    ++nodes;
    const int tv = evaluateTerminal(board);
    if (tv != kOngoing) return tv * kWinScore;
    if (depth == 0)     return heuristicEval(board);

    const std::vector<int> moves = board.getLegalMovesOrdered();
    if (board.current_player == 'X') {
        int v = std::numeric_limits<int>::min();
        for (int col : moves) {
            v = std::max(v, abSearch(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v >= beta) break;
            alpha = std::max(alpha, v);
        }
        return v;
    } else {
        int v = std::numeric_limits<int>::max();
        for (int col : moves) {
            v = std::min(v, abSearch(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v <= alpha) break;
            beta = std::min(beta, v);
        }
        return v;
    }
}

int main() {
    Board board;
    std::cin >> board.current_player;
    board.grid.assign(kRows, std::string());
    for (int r = 0; r < kRows; ++r) std::cin >> board.grid[r];

    const std::vector<int> moves = board.getLegalMovesOrdered();
    if (moves.empty()) return 1;

    int alpha    = std::numeric_limits<int>::min();
    int beta     = std::numeric_limits<int>::max();
    int best_col = moves[0];
    int best_val = (board.current_player == 'X')
                   ? std::numeric_limits<int>::min()
                   : std::numeric_limits<int>::max();
    long long nodes = 0;

    for (int col : moves) {
        int v = abSearch(board.applyMove(col), alpha, beta, kAiDepth - 1, nodes);
        if (board.current_player == 'X') {
            if (v > best_val) { best_val = v; best_col = col; }
            alpha = std::max(alpha, best_val);
        } else {
            if (v < best_val) { best_val = v; best_col = col; }
            beta = std::min(beta, best_val);
        }
    }

    std::cout << best_col << '\n';
    return 0;
}
