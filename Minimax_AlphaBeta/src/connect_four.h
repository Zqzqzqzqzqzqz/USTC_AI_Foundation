#pragma once
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

constexpr int kRows = 6;
constexpr int kCols = 7;
constexpr int kXWin = 1;
constexpr int kDraw = 0;
constexpr int kOWin = -1;
constexpr int kOngoing = 2;


constexpr int kMoveOrder[kCols] = {3, 2, 4, 1, 5, 0, 6};// 中心列优先的落子顺序，利于 alpha-beta 剪枝

struct Board {
    char current_player = 'X';
    std::vector<std::string> grid;

    bool loadFromFile(const std::string& path) {
        std::ifstream fin(path);
        if (!fin) return false;
        fin >> current_player;
        grid.assign(kRows, std::string());
        for (int r = 0; r < kRows; ++r) fin >> grid[r];
        return true;
    }

    std::vector<int> getLegalMoves() const {    // 获取合法落子列
        std::vector<int> moves;
        for (int c = 0; c < kCols; ++c)
            if (grid[0][c] == '.') moves.push_back(c);
        return moves;
    }

    
    std::vector<int> getLegalMovesOrdered() const {   // 获取合法落子列，按照 kMoveOrder 中的顺序返回
        std::vector<int> moves;
        for (int c : kMoveOrder)
            if (grid[0][c] == '.') moves.push_back(c);
        return moves;
    }

    Board applyMove(int col) const {    // 在 col 列落子
        Board next = *this;
        for (int r = kRows - 1; r >= 0; --r) {
            if (next.grid[r][col] == '.') {
                next.grid[r][col] = current_player;
                next.current_player = (current_player == 'X' ? 'O' : 'X');
                return next;
            }
        }
        return next;
    }

    bool isFull() const {
        for (int c = 0; c < kCols; ++c)
            if (grid[0][c] == '.') return false;
        return true;
    }

    int emptyCount() const {    // 返回棋盘上空格的数量
        int n = 0;
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c)
                if (grid[r][c] == '.') ++n;
        return n;
    }

    void print() const {
        std::cout << "  0 1 2 3 4 5 6\n";
        for (int r = 0; r < kRows; ++r) {
            std::cout << "| ";
            for (int c = 0; c < kCols; ++c)
                std::cout << grid[r][c] << ' ';
            std::cout << "|\n";
        }
        std::cout << "+-+-+-+-+-+-+-+\n";
    }
};

inline bool hasConnectFour(const Board& board, char piece) {
    const int dr[4] = {1, 0, 1, 1};
    const int dc[4] = {0, 1, 1, -1};
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (board.grid[r][c] != piece) continue;
            for (int dir = 0; dir < 4; ++dir) {
                bool ok = true;
                for (int step = 1; step < 4; ++step) {
                    int nr = r + dr[dir] * step;
                    int nc = c + dc[dir] * step;
                    if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols ||
                        board.grid[nr][nc] != piece) {
                        ok = false; break;
                    }
                }
                if (ok) return true;
            }
        }
    }
    return false;
}

inline int evaluateTerminal(const Board& board) {   
    if (hasConnectFour(board, 'X')) return kXWin;
    if (hasConnectFour(board, 'O')) return kOWin;
    if (board.isFull()) return kDraw;
    return kOngoing;
}


inline int scoreWindow(int x, int o, int e) {   //对一个长度为4的格子窗口评分
    if (x > 0 && o > 0) return 0;
    if (x == 3 && e == 1) return 100;
    if (o == 3 && e == 1) return -100;
    if (x == 2 && e == 2) return 5;
    if (o == 2 && e == 2) return -5;
    return 0;
}

inline int heuristicEval(const Board& board) {  //对整个棋盘做启发式评估
    int score = 0;
    const int dr[4] = {1, 0, 1, 1};
    const int dc[4] = {0, 1, 1, -1};
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            for (int dir = 0; dir < 4; ++dir) {
                int xc = 0, oc = 0, ec = 0;
                bool valid = true;
                for (int step = 0; step < 4; ++step) {
                    int nr = r + dr[dir] * step;
                    int nc = c + dc[dir] * step;
                    if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) {
                        valid = false; break;
                    }
                    char ch = board.grid[nr][nc];
                    if (ch == 'X') ++xc;
                    else if (ch == 'O') ++oc;
                    else ++ec;
                }
                if (valid) score += scoreWindow(xc, oc, ec);
            }
        }
    }
    for (int r = 0; r < kRows; ++r) {
        if (board.grid[r][3] == 'X') score += 3;
        else if (board.grid[r][3] == 'O') score -= 3;
    }
    return score;
}
