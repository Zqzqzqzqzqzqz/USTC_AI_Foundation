from __future__ import annotations

import math
from collections import deque
from pathlib import Path


def read_scene(path):
    path = Path(path)
    lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    x, y, z = map(int, lines[0].split())
    start = tuple(map(int, lines[1].split()))
    goal = tuple(map(int, lines[2].split()))
    k = int(lines[3])
    obstacles = []
    for i in range(k):
        parts = lines[4 + i].split()
        obstacles.append({
            "min": tuple(map(int, parts[:3])),
            "max": tuple(map(int, parts[3:6])),
            "label": parts[6] if len(parts) > 6 else "object",
        })
    return {"size": (x, y, z), "start": start, "goal": goal, "obstacles": obstacles}


def build_occupancy(scene):
    x, y, z = scene["size"]
    occ = [[[False for _ in range(z)] for _ in range(y)] for _ in range(x)]
    for obs in scene["obstacles"]:
        x1, y1, z1 = obs["min"]
        x2, y2, z2 = obs["max"]
        for ix in range(max(0, x1), min(x, x2 + 1)):
            for iy in range(max(0, y1), min(y, y2 + 1)):
                for iz in range(max(0, z1), min(z, z2 + 1)):
                    occ[ix][iy][iz] = True
    return occ


def parse_output(path):
    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(str(path))
    lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not lines:
        raise ValueError("empty output")
    if lines[0] == "-1":
        return {"path_length": -1, "num_points": 0, "points": []}
    if len(lines) < 2:
        raise ValueError("missing num_points")
    path_length_value = float(lines[0])
    num_points = int(lines[1])
    if len(lines) != num_points + 2:
        raise ValueError(f"expected {num_points} points, got {len(lines) - 2}")
    points = []
    for line in lines[2:]:
        parts = line.split()
        if len(parts) != 3:
            raise ValueError(f"bad point line: {line}")
        points.append(tuple(float(v) for v in parts))
    return {"path_length": path_length_value, "num_points": num_points, "points": points}


def is_inside_grid(point, scene):
    x, y, z = scene["size"]
    return 0 <= point[0] < x and 0 <= point[1] < y and 0 <= point[2] < z


def point_to_cell(point):
    return tuple(int(math.floor(v + 1e-9)) for v in point)


def is_occupied_cell(cell, occupancy):
    x, y, z = cell
    if x < 0 or y < 0 or z < 0 or x >= len(occupancy) or y >= len(occupancy[0]) or z >= len(occupancy[0][0]):
        return True
    return occupancy[x][y][z]


def point_collision_free(point, scene, occupancy):
    if not is_inside_grid(point, scene):
        return False
    return not is_occupied_cell(point_to_cell(point), occupancy)


def segment_collision_free(p1, p2, scene, occupancy, step=0.25):
    dist = math.dist(p1, p2)
    samples = max(1, int(math.ceil(dist / step)))
    for i in range(samples + 1):
        t = i / samples
        p = tuple(p1[j] * (1.0 - t) + p2[j] * t for j in range(3))
        if not point_collision_free(p, scene, occupancy):
            return False
    return True


def path_length(points):
    return sum(math.dist(points[i - 1], points[i]) for i in range(1, len(points)))


def base_neighbors(cell):
    x, y, z = cell
    for dx, dy, dz in [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]:
        yield (x + dx, y + dy, z + dz)


def base_move_valid(a, b, scene, occupancy):
    if not all(abs(a[i] - b[i]) <= 1 for i in range(3)):
        return False
    if sum(abs(a[i] - b[i]) for i in range(3)) != 1:
        return False
    return point_collision_free(b, scene, occupancy)


def bfs_shortest_length(scene, occupancy):
    start = tuple(scene["start"])
    goal = tuple(scene["goal"])
    if not point_collision_free(start, scene, occupancy) or not point_collision_free(goal, scene, occupancy):
        return None
    q = deque([start])
    dist = {start: 0}
    while q:
        cur = q.popleft()
        if cur == goal:
            return dist[cur]
        for nxt in base_neighbors(cur):
            if nxt not in dist and point_collision_free(nxt, scene, occupancy):
                dist[nxt] = dist[cur] + 1
                q.append(nxt)
    return None


def _resample_by_arclength(points, spacing=0.5):
    if len(points) < 2:
        return points[:]
    total = path_length(points)
    if total <= 1e-9:
        return [points[0], points[-1]]
    targets = [i * spacing for i in range(int(total / spacing) + 1)]
    if targets[-1] < total:
        targets.append(total)
    out = []
    seg_i = 1
    seg_start_len = 0.0
    cur_seg_len = math.dist(points[0], points[1])
    for target in targets:
        while seg_i < len(points) - 1 and seg_start_len + cur_seg_len < target:
            seg_start_len += cur_seg_len
            seg_i += 1
            cur_seg_len = math.dist(points[seg_i - 1], points[seg_i])
        if cur_seg_len <= 1e-9:
            out.append(points[seg_i])
        else:
            t = (target - seg_start_len) / cur_seg_len
            out.append(tuple(points[seg_i - 1][j] * (1 - t) + points[seg_i][j] * t for j in range(3)))
    return out


def smoothness_score(points, scene, occupancy):
    if len(points) < 2:
        return 0.0
    for p in points:
        if not point_collision_free(p, scene, occupancy):
            return 0.0
    for a, b in zip(points, points[1:]):
        if not segment_collision_free(a, b, scene, occupancy, step=0.25):
            return 0.0
    sampled = _resample_by_arclength(points, spacing=0.5)
    angles = []
    for i in range(1, len(sampled) - 1):
        v1 = [sampled[i][j] - sampled[i - 1][j] for j in range(3)]
        v2 = [sampled[i + 1][j] - sampled[i][j] for j in range(3)]
        n1 = math.sqrt(sum(v * v for v in v1))
        n2 = math.sqrt(sum(v * v for v in v2))
        if n1 <= 1e-9 or n2 <= 1e-9:
            continue
        c = max(-1.0, min(1.0, sum(v1[j] * v2[j] for j in range(3)) / (n1 * n2)))
        angles.append(math.acos(c))
    mean_turn_angle = sum(angles) / len(angles) if angles else 0.0
    length = path_length(points)
    straight_dist = math.dist(points[0], points[-1])
    length_ratio = length / max(straight_dist, 1e-6)
    return 100.0 / (1.0 + 2.0 * mean_turn_angle + 0.05 * max(0.0, length_ratio - 1.0))
