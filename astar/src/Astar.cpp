#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <vector>

using namespace std;

struct Point {
    int x, y, z;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
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
    scene.obstacles.resize(K);  // K个障碍物
    for (int i = 0; i < K; ++i) {
        fin >> scene.obstacles[i].x1 >> scene.obstacles[i].y1 >> scene.obstacles[i].z1
            >> scene.obstacles[i].x2 >> scene.obstacles[i].y2 >> scene.obstacles[i].z2
            >> scene.obstacles[i].label;
    }
    return true;
}

vector<vector<vector<int>>> buildOccupancy(const Scene& scene) { //返回三维动态数组
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

bool inside(const Scene& scene, const Point& p) { // 判断点p是否在场景内
    return p.x >= 0 && p.x < scene.X && p.y >= 0 && p.y < scene.Y && p.z >= 0 && p.z < scene.Z;
}

bool freeCell(const Scene& scene, const vector<vector<vector<int>>>& occ, const Point& p) { //碰撞检测
    return inside(scene, p) && occ[p.x][p.y][p.z] == 0; 
}

int flattenId(const Scene& scene, const Point& p) { // 将三维坐标转换为一维id
    return (p.x * scene.Y + p.y) * scene.Z + p.z;
}

Point pointFromId(const Scene& scene, int id) { // 将一维id转换为三维坐标
    return {id / (scene.Y * scene.Z), (id / scene.Z) % scene.Y, id % scene.Z};
}

int manhattan(const Point& a, const Point& b) {
    // TODO 1: 实现3D曼哈顿距离启发式函数。
    // 提示: |x1-x2| + |y1-y2| + |z1-z2|
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    int dz = abs(a.z - b.z);
    return dx + dy + dz;
}

vector<Point> getNeighbors6(const Point& p) {
    // TODO 2: 生成3D 6邻域。
    // 提示: 每次只改变x/y/z中的一个坐标，变化量为+1或-1。
    return{
        {p.x + 1, p.y, p.z},
        {p.x - 1, p.y, p.z},
        {p.x, p.y + 1, p.z},
        {p.x, p.y - 1, p.z},
        {p.x, p.y, p.z + 1},
        {p.x, p.y, p.z - 1}
    };
}

vector<Point> reconstructPath(const Scene& scene, const vector<int>& parent) {
    vector<Point> path;
    Point cur = scene.goal;
    while (!(cur == scene.start)) {
        path.push_back(cur);
        int pid = parent[flattenId(scene, cur)];
        if (pid < 0) return {};
        cur = pointFromId(scene, pid);
    }
    path.push_back(scene.start);
    reverse(path.begin(), path.end());
    return path;
}

vector<Point> astar(const Scene& scene) {
    
    // TODO 3: 实现A*主循环。
    // 要求:
    // 1. g(n)表示从起点到当前点的真实代价，每一步代价为1。
    // 2. h(n)调用manhattan(n, scene.goal)。
    // 3. f(n)=g(n)+h(n)，每次从优先队列中取f最小的节点扩展。
    // 4. 只扩展freeCell为true的6邻域点。
    // 5. 用parent[flattenId(scene, nxt)]记录父节点。
    // 6. 找到goal后调用reconstructPath(scene, parent)返回路径。
    //
    // 下面这些变量已经准备好，可直接使用。
    auto occ = buildOccupancy(scene);
    if (!freeCell(scene, occ, scene.start) || !freeCell(scene, occ, scene.goal)) return {}; 

    const int total = scene.X * scene.Y * scene.Z;
    vector<int> g(total, numeric_limits<int>::max());
    vector<int> parent(total, -1);
    struct Node {
        int f;
        int id;
        // 小根堆
        bool operator<(const Node& other) const {
            return f > other.f;
        }
    };
    priority_queue<Node> pq; // 小根堆
    int  start_id = flattenId(scene, scene.start);
    g[start_id] = 0;
    pq.push({manhattan(scene.start, scene.goal), start_id});
    while(!pq.empty()) {
        Node curr_node = pq.top();
        pq.pop();
        Point curr = pointFromId(scene, curr_node.id); 

        if(curr == scene.goal) {    // 到达终点
            return reconstructPath(scene, parent);
        }

        if(curr_node.f>g[curr_node.id]+manhattan(curr, scene.goal)) { // 已经有更优的路径
            continue;
        }

        //扩展邻居节点
        for(const Point& nxt : getNeighbors6(curr)){
            if(freeCell(scene, occ, nxt)) {
                int nxt_id = flattenId(scene, nxt);
                int tentative_g = g[curr_node.id]+1;
                if(tentative_g < g[nxt_id]) { // 找到更优的路径
                    g[nxt_id] = tentative_g;
                    parent[nxt_id] = curr_node.id;
                    int f = tentative_g + manhattan(nxt, scene.goal);
                    pq.push({f, nxt_id});
                }
            }
                
        }

    }

    return {};
}

void writePath(const string& path, const vector<Point>& result) {
    ofstream fout(path);
    if (!fout) return;
    if (result.empty()) {
        fout << -1 << "\n";
        return;
    }
    fout << result.size() - 1 << "\n";
    fout << result.size() << "\n";
    for (const auto& p : result) {
        fout << p.x << " " << p.y << " " << p.z << "\n";
    }
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
    vector<Point> result = astar(scene);
    writePath(argv[2], result);
    return 0;
}
