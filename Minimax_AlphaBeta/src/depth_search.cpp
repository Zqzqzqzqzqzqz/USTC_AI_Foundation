// 深度限制 alpha-beta + 启发式评估函数，支持空位数更多的局面。
//./depth_search <input_file> [depth] (depth 默认为 8)

// 输出格式（3 行）：
//   best_move
//   预测结果（X_WIN / O_WIN / DRAW / UNKNOWN）
//   展开节点数

#include "connect_four.h"


constexpr int kWinScore  =  1000000;
constexpr int kLossScore = -1000000;

int abDepth(const Board& board, int alpha, int beta, int depth, long long& nodes) {// alpha-beta 搜索，返回评估值
    ++nodes;

    const int tv = evaluateTerminal(board);
    if (tv != kOngoing) return tv * kWinScore; // 终局评估：X 赢返回正分，O 赢返回负分，平局返回 0
    if (depth == 0)     return heuristicEval(board); 

    const std::vector<int> moves = board.getLegalMovesOrdered();

    if (board.current_player == 'X') {
        int v = std::numeric_limits<int>::min();
        for (int col : moves) {// 对每个合法落子列，递归搜索下一层
            v = std::max(v, abDepth(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v >= beta) break;
            alpha = std::max(alpha, v);
        }
        return v;
    } else {// O 作为后手，寻找最小值
        int v = std::numeric_limits<int>::max();
        for (int col : moves) {
            v = std::min(v, abDepth(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v <= alpha) break;
            beta = std::min(beta, v);
        }
        return v;
    }
}


std::string interpretValue(int v) {
    if (v >= kWinScore / 2)  return "X_WIN";
    if (v <= kLossScore / 2) return "O_WIN";
    if (v == 0)              return "DRAW";
    return "UNKNOWN";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [depth]\n";
        return 1;
    }

    Board board;
    if (!board.loadFromFile(argv[1])) {
        std::cerr << "Failed to read: " << argv[1] << '\n';
        return 1;
    }

    const int depth = (argc == 3) ? std::stoi(argv[2]) : 8;
    std::cout << "空格数量: " << board.emptyCount() // 输出棋盘上空格的数量
              << "  搜索深度: " << depth << '\n';

    const std::vector<int> moves = board.getLegalMovesOrdered();
    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();
    int best_move  = moves[0];
    int best_value = (board.current_player == 'X')
                     ? std::numeric_limits<int>::min()
                     : std::numeric_limits<int>::max();
    long long nodes = 0;

    for (int col : moves) {// 对每个合法落子列，递归搜索下一层
        int v = abDepth(board.applyMove(col), alpha, beta, depth - 1, nodes);
        if (board.current_player == 'X') {
            if (v > best_value) { best_value = v; best_move = col; }
            alpha = std::max(alpha, best_value);
        } else {
            if (v < best_value) { best_value = v; best_move = col; }
            beta = std::min(beta, best_value);
        }
    }   

    std::cout << best_move << '\n';
    std::cout << interpretValue(best_value) << '\n';
    std::cout << nodes << '\n';
    return 0;
}
