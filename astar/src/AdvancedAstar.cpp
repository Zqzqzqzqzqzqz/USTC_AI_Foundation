#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>

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
    // 当两段方向完全一致时，cos(\theta)=1，平滑代价为 0；
    // 当方向相反（180度回头）时，cos(\theta)=-1，平滑代价为 2。
    double v1x = cur.x - prev.x, v1y = cur.y - prev.y, v1z = cur.z - prev.z;
    double v2x = nxt.x - cur.x,  v2y = nxt.y - cur.y,  v2z = nxt.z - cur.z;
    double n1 = sqrt(v1x * v1x + v1y * v1y + v1z * v1z);
    double n2 = sqrt(v2x * v2x + v2y * v2y + v2z * v2z);
    if (n1 < 1e-9 || n2 < 1e-9) return 0.0;          // 退化情形
    double cosTheta = (v1x * v2x + v1y * v2y + v1z * v2z) / (n1 * n2);
    if (cosTheta > 1.0) cosTheta = 1.0;//上限
    if (cosTheta < -1.0) cosTheta = -1.0;
    return 1.0 - cosTheta;                            // 范围 [0, 2]
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


//buildOccupancy 很慢，因此在 advancedPlan 内部一次性构建好膨胀后的占据栅格并复用。

// 使用预先构建的占据栅格做线段碰撞检测
static bool lineCollisionFreeWithOcc(const Point& a, const Point& b,
                                     const Scene& scene,
                                     const vector<vector<vector<int>>>& occ) {
    double d = heuristic(a, b);
    int steps = max(1, static_cast<int>(ceil(d / 0.25)));   // 0.25 步长
    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        Point p{a.x * (1.0 - t) + b.x * t,
                a.y * (1.0 - t) + b.y * t,
                a.z * (1.0 - t) + b.z * t};
        if (occupiedPoint(scene, occ, p)) return false;
    }
    return true;
}

// 障碍物膨胀：考虑夹爪体积/安全距离，把障碍物向外扩 inflate 个格点。
// 这样规划出来的路径会自动远离障碍物，更适合机械臂实际执行。
static vector<vector<vector<int>>> buildInflatedOccupancy(const Scene& scene, int inflate) {
    vector<vector<vector<int>>> occ(scene.X,
        vector<vector<int>>(scene.Y, vector<int>(scene.Z, 0)));
    for (const auto& o : scene.obstacles) {
        int x1 = max(0, o.x1 - inflate);
        int y1 = max(0, o.y1 - inflate);
        int z1 = max(0, o.z1 - inflate);
        int x2 = min(scene.X - 1, o.x2 + inflate);
        int y2 = min(scene.Y - 1, o.y2 + inflate);
        int z2 = min(scene.Z - 1, o.z2 + inflate);
        for (int x = x1; x <= x2; ++x)
            for (int y = y1; y <= y2; ++y)
                for (int z = z1; z <= z2; ++z)
                    occ[x][y][z] = 1;
    }
    return occ;
}

vector<Point> shortcutSmooth(const vector<Point>& path, const Scene& scene) {
    // TODO 4: 可选shortcut smoothing，删除可被无碰撞直线连接的中间点。
    // 贪心策略：从当前点出发，尽可能往后跳到一个仍能用直线无碰撞连接的最远点，
    // 把中间的点都删掉。这样可以显著减少路径中的"锯齿"。
    if (path.size() <= 2) return path;
    vector<Point> result;
    result.push_back(path.front());
    size_t i = 0;
    while (i < path.size() - 1) {
        size_t j = path.size() - 1;                  // 从最远端开始尝试
        while (j > i + 1) {
            if (lineCollisionFree(path[i], path[j], scene)) break;
            --j;
        }
        result.push_back(path[j]);
        i = j;
    }
    return result;
}

vector<Point> interpolatePath(const vector<Point>& path) {
    // TODO 5: 可选线性插值或其他轨迹后处理。
    // 在相邻两个路径点之间做线性插值，使得任意两个相邻输出点之间的间距大约为 step。
    // 这样路径更连续、采样更均匀，便于机械臂跟踪执行。
    if (path.size() < 2) return path;
    const double step = 0.5;                          // 每段插值间距
    vector<Point> result;
    result.push_back(path.front());
    for (size_t i = 1; i < path.size(); ++i) {
        const Point& a = path[i - 1];
        const Point& b = path[i];
        double d = heuristic(a, b);
        int n = max(1, static_cast<int>(ceil(d / step)));
        for (int k = 1; k <= n; ++k) {
            double t = static_cast<double>(k) / n;
            Point p{a.x * (1.0 - t) + b.x * t,
                    a.y * (1.0 - t) + b.y * t,
                    a.z * (1.0 - t) + b.z * t};
            result.push_back(p);
        }
    }
    return result;
}

vector<Point> advancedPlan(const Scene& scene) {
    // TODO 2: 使用3D 26邻域搜索。
    // TODO 3: 使用 f = g + h + lambda_smooth * cost_smooth，并回溯离散路径。
    // 进阶部分不要求最短路，可以优先考虑路径平滑和可执行性。


     const int    inflate       = 1;        // 障碍物膨胀半径，等价于安全距离（提高内容 4）
    const double lambdaSmooth  = 0.6;      // 平滑代价权重，越大路径越平直、越绕远
    // ------------------

    // 构建膨胀后的占据栅格（考虑夹爪体积和安全距离）
    auto occ = buildInflatedOccupancy(scene, inflate);

    // 起点和终点取整
    int sx = static_cast<int>(round(scene.start.x));
    int sy = static_cast<int>(round(scene.start.y));
    int sz = static_cast<int>(round(scene.start.z));
    int gx = static_cast<int>(round(scene.goal.x));
    int gy = static_cast<int>(round(scene.goal.y));
    int gz = static_cast<int>(round(scene.goal.z));

    auto inBound = [&](int x, int y, int z) {
        return x >= 0 && x < scene.X && y >= 0 && y < scene.Y && z >= 0 && z < scene.Z;
    };
    if (!inBound(sx, sy, sz) || !inBound(gx, gy, gz)) return {};

    // 起点/终点本身被障碍物覆盖时，先用未膨胀的原始占据栅格兜底，
    // 否则起点或终点紧贴障碍物时无法规划。
    auto rawOcc = buildOccupancy(scene);
    if (rawOcc[sx][sy][sz] || rawOcc[gx][gy][gz]) return {};
    if (occ[sx][sy][sz]) occ[sx][sy][sz] = 0;
    if (occ[gx][gy][gz]) occ[gx][gy][gz] = 0;

    // 26 邻域方向，及对应的步长（欧氏距离）
    vector<tuple<int, int, int, double>> dirs;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                double cost = sqrt(double(dx * dx + dy * dy + dz * dz));
                dirs.emplace_back(dx, dy, dz, cost);
            }

    // 把 (x,y,z) 映射成一维索引，用于哈希表存储
    auto idx = [&](int x, int y, int z) {
        return ((long long)x * scene.Y + y) * scene.Z + z;
    };

    // A* 节点。注意比较时使用 f 值，f = g + h + lambda * smooth_accumulated。
    struct Node {
        long long id;
        double f;
        bool operator>(const Node& o) const { return f > o.f; }
    };

    unordered_map<long long, double> gScore;          // 累计的 g + lambda*smooth
    unordered_map<long long, long long> parent;       // 父节点
    priority_queue<Node, vector<Node>, greater<Node>> open;

    long long startId = idx(sx, sy, sz);
    long long goalId  = idx(gx, gy, gz);
    gScore[startId] = 0.0;
    parent[startId] = -1;
    Point goalP{double(gx), double(gy), double(gz)};
    open.push({startId, heuristic({double(sx), double(sy), double(sz)}, goalP)});

    // 用于在扩展时计算平滑代价：需要知道当前节点的父节点位置
    auto decode = [&](long long id, int& x, int& y, int& z) {
        z = id % scene.Z;
        long long t = id / scene.Z;
        y = t % scene.Y;
        x = t / scene.Y;
    };

    bool found = false;
    while (!open.empty()) {
        Node cur = open.top(); open.pop();
        if (cur.id == goalId) { found = true; break; }

        int cx, cy, cz; decode(cur.id, cx, cy, cz);

        // 取父节点，用于计算平滑代价
        long long pid = parent[cur.id];
        Point prevP;
        bool hasPrev = (pid != -1);
        if (hasPrev) {
            int px, py, pz; decode(pid, px, py, pz);
            prevP = {double(px), double(py), double(pz)};
        }
        Point curP{double(cx), double(cy), double(cz)};

        double curG = gScore[cur.id];
        // 重新计算 f，跳过已经被更优解淘汰的过期节点
        double curF = curG + heuristic(curP, goalP);
        if (cur.f > curF + 1e-9) {
            // 这是一条过期记录，但因为我们没有维护严格 closed 集合，
        }

        for (auto& d : dirs) {
            int dx, dy, dz; double mvCost;
            tie(dx, dy, dz, mvCost) = d;
            int nx = cx + dx, ny = cy + dy, nz = cz + dz;
            if (!inBound(nx, ny, nz)) continue;
            if (occ[nx][ny][nz]) continue;            // 撞到（膨胀后的）障碍物

            // 26 邻域的对角移动需要确保经过的"边/角"附近也是空的，否则会出现夹缝穿越。
            // 要求移动方向上经过的几个格子都不被占据。
            // 对于 dx,dy,dz 中非零分量，逐一检查替换为 0 后的格子。
            bool diagonalOk = true;
            int nonZero = (dx != 0) + (dy != 0) + (dz != 0);
            if (nonZero >= 2) {
                // 检查所有"投影到面"的相邻格子
                int axes[3] = {dx, dy, dz};
                int base[3] = {cx, cy, cz};
                int delta[3] = {dx, dy, dz};
                // 枚举掩码，至少保留一个 0 分量
                for (int mask = 1; mask < 7; ++mask) {
                    int p[3] = {base[0], base[1], base[2]};
                    int cnt = 0;
                    for (int k = 0; k < 3; ++k)
                        if (mask & (1 << k)) { p[k] += delta[k]; cnt++; }
                    if (cnt == 0 || cnt == nonZero) continue;   // 跳过自身和终点
                    if (!inBound(p[0], p[1], p[2])) continue;
                    if (occ[p[0]][p[1]][p[2]]) { diagonalOk = false; break; }
                }
                (void)axes;
            }
            if (!diagonalOk) continue;

            // 计算平滑代价
            Point nxtP{double(nx), double(ny), double(nz)};
            double smooth = 0.0;
            if (hasPrev) smooth = computeSmoothCost(prevP, curP, nxtP);

            double tentG = curG + mvCost + lambdaSmooth * smooth;
            long long nid = idx(nx, ny, nz);
            auto it = gScore.find(nid);
            if (it == gScore.end() || tentG < it->second - 1e-9) {
                gScore[nid] = tentG;
                parent[nid] = cur.id;
                double f = tentG + heuristic(nxtP, goalP);
                open.push({nid, f});
            }
        }
    }

    if (!found) return {};

    // 回溯路径
    vector<Point> path;
    long long cur = goalId;
    while (cur != -1) {
        int x, y, z; decode(cur, x, y, z);
        path.push_back({double(x), double(y), double(z)});
        auto it = parent.find(cur);
        if (it == parent.end()) break;
        cur = it->second;
    }
    reverse(path.begin(), path.end());
    return path;
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
