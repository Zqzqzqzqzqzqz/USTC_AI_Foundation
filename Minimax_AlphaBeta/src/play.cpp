// 命令行人机对弈
//   ./play              人执 X 先手
//   ./play O            人执 O，AI 执 X 先手

#include "connect_four.h"

constexpr int kWinScore  =  1000000;
constexpr int kLossScore = -1000000;
constexpr int kAiDepth   = 8;

int abDepth(const Board& board, int alpha, int beta, int depth, long long& nodes) {// alpha-beta 搜索，返回评估值
    ++nodes;
    const int tv = evaluateTerminal(board);
    if (tv != kOngoing) return tv * kWinScore;
    if (depth == 0)     return heuristicEval(board);

    const std::vector<int> moves = board.getLegalMovesOrdered();
    if (board.current_player == 'X') {
        int v = std::numeric_limits<int>::min();
        for (int col : moves) {
            v = std::max(v, abDepth(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v >= beta) break;
            alpha = std::max(alpha, v);
        }
        return v;
    } else {
        int v = std::numeric_limits<int>::max();
        for (int col : moves) {
            v = std::min(v, abDepth(board.applyMove(col), alpha, beta, depth - 1, nodes));
            if (v <= alpha) break;
            beta = std::min(beta, v);
        }
        return v;
    }
}

int aiChooseMove(const Board& board) {
    const std::vector<int> moves = board.getLegalMovesOrdered();
    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();
    int best_move  = moves[0];
    int best_value = (board.current_player == 'X')
                     ? std::numeric_limits<int>::min()
                     : std::numeric_limits<int>::max();
    long long nodes = 0;

    for (int col : moves) {
        int v = abDepth(board.applyMove(col), alpha, beta, kAiDepth - 1, nodes);
        if (board.current_player == 'X') {
            if (v > best_value) { best_value = v; best_move = col; }
            alpha = std::max(alpha, best_value);
        } else {
            if (v < best_value) { best_value = v; best_move = col; }
            beta = std::min(beta, best_value);
        }
    }

    std::cout << "[AI searched " << nodes << " nodes, eval=" << best_value << "]\n";
    return best_move;
}

int humanChooseMove(const Board& board) {// 从标准输入读取人类玩家的落子列，进行合法性检查
    while (true) {
        std::cout << "请落子 (0-6): ";
        int col;
        if (!(std::cin >> col)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效。\n";
            continue;
        }
        if (col < 0 || col >= kCols) {
            std::cout << "列超出范围。\n";
            continue;
        }
        if (board.grid[0][col] != '.') {
            std::cout << "第" << col << "列满了.\n";
            continue;
        }
        return col;
    }
}

int main(int argc, char* argv[]) {
    char human_piece = 'X';
    if (argc == 2) {
        std::string arg(argv[1]);
        if (arg == "O" || arg == "o") human_piece = 'O';
    }
    char ai_piece = (human_piece == 'X') ? 'O' : 'X';

    Board board;
    board.current_player = 'X';
    board.grid.assign(kRows, std::string(kCols, '.'));

    std::cout << "=== Connect Four: Human (" << human_piece
              << ") vs AI (" << ai_piece << ") ===\n";
    std::cout << "Columns: 0 1 2 3 4 5 6\n\n";

    while (true) {
        board.print();

        const int tv = evaluateTerminal(board);
        if (tv != kOngoing) {
            if (tv == kXWin) {
                std::cout << ((human_piece == 'X') ? "恭喜你赢了!\n" : "AI赢了!\n");
            } else if (tv == kOWin) {
                std::cout << ((human_piece == 'O') ? "恭喜你赢了!\n" : "AI赢了!\n");
            } else {
                std::cout << "平局!\n";
            }
            break;
        }

        int col;
        if (board.current_player == human_piece) {
            col = humanChooseMove(board);
        } else {
            std::cout << "AI正在思考：depth=" << kAiDepth << ")...\n";
            col = aiChooseMove(board);
            std::cout << "提示：AI在第 " << col << "列落子\n";
        }
        board = board.applyMove(col);
        std::cout << '\n';
    }
    return 0;
}
