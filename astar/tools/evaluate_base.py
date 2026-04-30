from __future__ import annotations

import argparse
from pathlib import Path

from common import base_move_valid, bfs_shortest_length, build_occupancy, parse_output, point_collision_free, read_scene


def is_int_point(p):
    return all(abs(v - round(v)) <= 1e-9 for v in p)


def evaluate_case(input_path, output_dir):
    scene = read_scene(input_path)
    occ = build_occupancy(scene)
    output_path = output_dir / input_path.name.replace("input_", "output_")
    try:
        result = parse_output(output_path)
    except Exception as exc:
        return 0, f"{input_path.name}: output error: {exc}"
    points = result["points"]
    shortest = bfs_shortest_length(scene, occ)
    if result["path_length"] < 0:
        return 0, f"{input_path.name}: no path output, but shortest={shortest}"
    if not points:
        return 0, f"{input_path.name}: empty path"
    int_points = [tuple(int(round(v)) for v in p) for p in points]
    if not all(is_int_point(p) for p in points):
        return 0, f"{input_path.name}: non-integer point"
    if int_points[0] != tuple(scene["start"]):
        return 0, f"{input_path.name}: wrong start {int_points[0]}"
    if int_points[-1] != tuple(scene["goal"]):
        return 0, f"{input_path.name}: wrong goal {int_points[-1]}"
    for p in int_points:
        if not point_collision_free(p, scene, occ):
            return 0, f"{input_path.name}: invalid or occupied point {p}"
    for a, b in zip(int_points, int_points[1:]):
        if not base_move_valid(a, b, scene, occ):
            return 0, f"{input_path.name}: invalid 6-neighbor move {a}->{b}"
    if abs(result["path_length"] - (len(points) - 1)) > 1e-9:
        return 0, f"{input_path.name}: path_length must equal num_points - 1"
    if shortest is None or int(result["path_length"]) != shortest:
        return 0, f"{input_path.name}: not shortest, got {result['path_length']}, expected {shortest}"
    return 60, f"{input_path.name}: OK shortest={shortest}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("input"))
    parser.add_argument("--output", type=Path, default=Path("base_output"))
    args = parser.parse_args()

    scores = []
    for input_path in sorted(args.input.glob("input_*.txt")):
        score, msg = evaluate_case(input_path, args.output)
        scores.append(score)
        print(f"[{score:2d}/60] {msg}")
    avg = sum(scores) / len(scores) if scores else 0.0
    print(f"Average base score: {avg:.2f} / 60")


if __name__ == "__main__":
    main()
