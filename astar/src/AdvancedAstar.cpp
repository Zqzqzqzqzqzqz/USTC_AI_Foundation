#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct Point {
    double x, y, z;
};

struct Cuboid {
    int x1, y1, z1, x2, y2, z2;
    string label;
};

struct Scene {
    int X = 0, Y = 0, Z = 0;
    Point start{}, goal{};
    vector<Cuboid> obstacles;
};

bool readScene(const string& path, Scene& scene) {
    ifstream fin(path);
    if (!fin) return false;
    int K = 0;
    fin >> scene.X >> scene.Y >> scene.Z;
    fin >> scene.start.x >> scene.start.y >> scene.start.z;
    fin >> scene.goal.x >> scene.goal.y >> scene.goal.z;
    fin >> K;
    scene.obstacles.resize(K);
    for (int i = 0; i < K; ++i) {
        fin >> scene.obstacles[i].x1 >> scene.obstacles[i].y1 >> scene.obstacles[i].z1
            >> scene.obstacles[i].x2 >> scene.obstacles[i].y2 >> scene.obstacles[i].z2
            >> scene.obstacles[i].label;
    }
    return true;
}

vector<vector<vector<int>>> buildOccupancy(const Scene& scene) {
    vector<vector<vector<int>>> occ(scene.X, vector<vector<int>>(scene.Y, vector<int>(scene.Z, 0)));
    for (const auto& o : scene.obstacles) {
        for (int x = max(0, o.x1); x <= min(scene.X - 1, o.x2); ++x) {
            for (int y = max(0, o.y1); y <= min(scene.Y - 1, o.y2); ++y) {
                for (int z = max(0, o.z1); z <= min(scene.Z - 1, o.z2); ++z) {
                    occ[x][y][z] = 1;
                }
            }
        }
    }
    return occ;
}

bool inside(const Scene& scene, const Point& p) {
    return p.x >= 0 && p.x < scene.X && p.y >= 0 && p.y < scene.Y && p.z >= 0 && p.z < scene.Z;
}

bool occupiedPoint(const Scene& scene, const vector<vector<vector<int>>>& occ, const Point& p) {
    if (!inside(scene, p)) return true;
    int x = static_cast<int>(floor(p.x + 1e-9));
    int y = static_cast<int>(floor(p.y + 1e-9));
    int z = static_cast<int>(floor(p.z + 1e-9));
    return occ[x][y][z] != 0;
}

double heuristic(const Point& a, const Point& b) {
    // 已提供欧氏距离启发式，也可以改成其他合理启发式。
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

double computeSmoothCost(const Point& prev, const Point& cur, const Point& nxt) {
    // TODO 1: 设计平滑代价，例如1-cos(theta)，惩罚频繁转弯和方向突变。
    (void)prev;
    (void)cur;
    (void)nxt;
    return 0.0;
}

bool lineCollisionFree(const Point& a, const Point& b, const Scene& scene) {
    auto occ = buildOccupancy(scene);
    double d = heuristic(a, b);
    int steps = max(1, static_cast<int>(ceil(d / 0.25)));
    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        Point p{a.x * (1.0 - t) + b.x * t, a.y * (1.0 - t) + b.y * t, a.z * (1.0 - t) + b.z * t};
        if (occupiedPoint(scene, occ, p)) return false;
    }
    return true;
}

vector<Point> shortcutSmooth(const vector<Point>& path, const Scene& scene) {
    // TODO 4: 可选shortcut smoothing，删除可被无碰撞直线连接的中间点。
    (void)scene;
    return path;
}

vector<Point> interpolatePath(const vector<Point>& path) {
    // TODO 5: 可选线性插值或其他轨迹后处理。
    return path;
}

vector<Point> advancedPlan(const Scene& scene) {
    auto occ = buildOccupancy(scene);
    (void)occ;
    // TODO 2: 使用3D 26邻域搜索。
    // TODO 3: 使用 f = g + h + lambda_smooth * cost_smooth，并回溯离散路径。
    // 进阶部分不要求最短路，可以优先考虑路径平滑和可执行性。
    (void)scene;
    return {};
}

void writePath(const string& path, const vector<Point>& result) {
    ofstream fout(path);
    if (!fout) return;
    if (result.empty()) {
        fout << -1 << "\n";
        return;
    }
    double length = 0.0;
    for (size_t i = 1; i < result.size(); ++i) length += heuristic(result[i - 1], result[i]);
    fout << length << "\n";
    fout << result.size() << "\n";
    for (const auto& p : result) fout << p.x << " " << p.y << " " << p.z << "\n";
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " input.txt output.txt\n";
        return 1;
    }
    Scene scene;
    if (!readScene(argv[1], scene)) {
        cerr << "Failed to read input.\n";
        return 1;
    }
    vector<Point> result = advancedPlan(scene);
    result = shortcutSmooth(result, scene);
    result = interpolatePath(result);
    writePath(argv[2], result);
    return 0;
}
