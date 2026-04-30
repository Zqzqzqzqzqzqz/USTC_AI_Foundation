#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr int kRows = 6;
constexpr int kCols = 7;
constexpr int kXWin = 1;
constexpr int kDraw = 0;
constexpr int kOWin = -1;
constexpr int kOngoing = 2;

struct SearchResult {
    int best_move = -1;
    int value = kDraw;
    long long minimax_nodes = 0;
    long long alphabeta_nodes = 0;
};

struct Board {
    char current_player = 'X';
    std::vector<std::string> grid;

    bool loadFromFile(const std::string& input_path) {
        std::ifstream fin(input_path);
        if (!fin) {
            return false;
        }

        fin >> current_player;
        grid.assign(kRows, std::string());
        for (int row = 0; row < kRows; ++row) {
            fin >> grid[row];
        }
        return true;
    }

    std::vector<int> getLegalMoves() const {
        std::vector<int> moves;
        for (int col = 0; col < kCols; ++col) {
            if (grid[0][col] == '.') {
                moves.push_back(col);
            }
        }
        return moves;
    }

    Board applyMove(int col) const {
        Board next = *this;
        for (int row = kRows - 1; row >= 0; --row) {
            if (next.grid[row][col] == '.') {
                next.grid[row][col] = current_player;
                next.current_player = (current_player == 'X' ? 'O' : 'X');
                return next;
            }
        }
        return next;
    }

    bool isFull() const {
        for (int col = 0; col < kCols; ++col) {
            if (grid[0][col] == '.') {
                return false;
            }
        }
        return true;
    }
};

bool hasConnectFour(const Board& board, char piece) {
    const int dr[4] = {1, 0, 1, 1};
    const int dc[4] = {0, 1, 1, -1};

    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            if (board.grid[row][col] != piece) {
                continue;
            }
            for (int dir = 0; dir < 4; ++dir) {
                bool ok = true;
                for (int step = 1; step < 4; ++step) {
                    const int nr = row + dr[dir] * step;
                    const int nc = col + dc[dir] * step;
                    if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols ||
                        board.grid[nr][nc] != piece) {
                        ok = false;
                        break;
                    }
                }
                if (ok) {
                    return true;
                }
            }
        }
    }
    return false;
}

int evaluateTerminal(const Board& board) {
    if (hasConnectFour(board, 'X')) {
        return kXWin;
    }
    if (hasConnectFour(board, 'O')) {
        return kOWin;
    }
    if (board.isFull()) {
        return kDraw;
    }
    return kOngoing;
}

int minimaxValue(const Board& board, long long& nodes) {
    ++nodes;

    const int terminal_value = evaluateTerminal(board);
    if (terminal_value != kOngoing) {
        return terminal_value;
    }

    // TODO:
    // 1. 生成 board 的所有合法落子列。
    // 2. 若当前行动方为 X，则返回所有后继状态值中的最大值。
    // 3. 若当前行动方为 O，则返回所有后继状态值中的最小值。
    int value = kDraw;
    return value;
}

int alphaBetaValue(const Board& board, int alpha, int beta, long long& nodes) {
    ++nodes;

    const int terminal_value = evaluateTerminal(board);
    if (terminal_value != kOngoing) {
        return terminal_value;
    }

    // TODO:
    // 1. 在 minimax 的基础上加入 alpha-beta 剪枝。
    // 2. 根据当前行动方初始化 value：
    //    - 若当前行动方为 X，可初始化为一个足够小的值。
    //    - 若当前行动方为 O，可初始化为一个足够大的值。
    // 3. 注意：后继展开顺序必须是从左到右。
    // 4. 注意：返回值必须与 minimax 完全一致。
    int value = kDraw;
    return value;
}

SearchResult solveWithMinimax(const Board& board) {
    SearchResult result;
    const std::vector<int> moves = board.getLegalMoves();

    // TODO:
    // 1. 枚举根节点每个合法动作，调用 minimaxValue 计算对应子状态的值。
    // 2. 若当前行动方为 X，选择值最大的动作；若为 O，选择值最小的动作。
    // 3. 若存在多个同样最优的动作，保留列编号最小的那个。
    // 4. 累计 result.minimax_nodes。
    return result;
}

SearchResult solveWithAlphaBeta(const Board& board) {
    SearchResult result;
    const std::vector<int> moves = board.getLegalMoves();

    // TODO:
    // 1. 枚举根节点每个合法动作，调用 alphaBetaValue 计算对应子状态的值。
    // 2. 在根节点也维护 alpha / beta。
    // 3. 若存在多个同样最优的动作，保留列编号最小的那个。
    // 4. 累计 result.alphabeta_nodes。
    return result;
}

std::string valueToString(int value) {
    if (value == kXWin) {
        return "X_WIN";
    }
    if (value == kOWin) {
        return "O_WIN";
    }
    return "DRAW";
}

bool writeAnswer(const std::string& output_path, const SearchResult& result) {
    std::ofstream fout(output_path);
    if (!fout) {
        return false;
    }

    fout << result.best_move << '\n';
    fout << valueToString(result.value) << '\n';
    fout << result.minimax_nodes << ' ' << result.alphabeta_nodes << '\n';
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    Board board;
    if (!board.loadFromFile(argv[1])) {
        std::cerr << "Failed to read input file: " << argv[1] << '\n';
        return 1;
    }

    SearchResult minimax_result = solveWithMinimax(board);
    SearchResult alphabeta_result = solveWithAlphaBeta(board);

    // TODO:
    // 两种搜索的 best_move 和 value 应完全一致。
    // 可在此加入一致性检查。

    SearchResult final_result;
    final_result.best_move = minimax_result.best_move;
    final_result.value = minimax_result.value;
    final_result.minimax_nodes = minimax_result.minimax_nodes;
    final_result.alphabeta_nodes = alphabeta_result.alphabeta_nodes;

    if (!writeAnswer(argv[2], final_result)) {
        std::cerr << "Failed to write output file: " << argv[2] << '\n';
        return 1;
    }
    return 0;
}
