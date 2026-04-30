from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from common import build_occupancy, parse_output, read_scene, segment_collision_free, smoothness_score


def close_point(a, b, eps=1e-6):
    return math.dist(a, b) <= eps


def advanced_valid(points, scene, occ):
    if len(points) < 2:
        return False, "too few points"
    if not close_point(points[0], tuple(float(v) for v in scene["start"])):
        return False, "wrong start"
    if not close_point(points[-1], tuple(float(v) for v in scene["goal"])):
        return False, "wrong goal"
    for a, b in zip(points, points[1:]):
        if not segment_collision_free(a, b, scene, occ, step=0.25):
            return False, f"collision segment {a}->{b}"
    return True, "OK"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("input"))
    parser.add_argument("--base_output", type=Path, default=Path("base_output"))
    parser.add_argument("--advanced_output", type=Path, default=Path("advanced_output"))
    args = parser.parse_args()

    scores = []
    for input_path in sorted(args.input.glob("input_*.txt")):
        scene = read_scene(input_path)
        occ = build_occupancy(scene)
        base_path = args.base_output / input_path.name.replace("input_", "output_")
        adv_path = args.advanced_output / input_path.name.replace("input_", "output_")
        record = {"case": input_path.name, "advanced_valid": False, "base_smoothness": 0.0, "advanced_smoothness": 0.0, "score": 0}
        try:
            base_points = parse_output(base_path)["points"]
            adv_points = parse_output(adv_path)["points"]
            valid, reason = advanced_valid(adv_points, scene, occ)
            base_score = smoothness_score(base_points, scene, occ)
            adv_score = smoothness_score(adv_points, scene, occ) if valid else 0.0
            score = 20 if valid and adv_score > base_score + 1e-6 else 0
            record.update({
                "advanced_valid": valid,
                "reason": reason,
                "base_smoothness": round(base_score, 3),
                "advanced_smoothness": round(adv_score, 3),
                "score": score,
            })
        except Exception as exc:
            record["reason"] = str(exc)
            score = 0
        scores.append(score)
        print(json.dumps(record, ensure_ascii=False, indent=2))
    avg = sum(scores) / len(scores) if scores else 0.0
    print(f"Average advanced score: {avg:.2f} / 20")


if __name__ == "__main__":
    main()
