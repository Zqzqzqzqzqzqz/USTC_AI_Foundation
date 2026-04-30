from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def run(cmd, cwd):
    print("+", " ".join(str(c) for c in cmd))
    subprocess.run(cmd, cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    (root / "base_output").mkdir(exist_ok=True)
    (root / "advanced_output").mkdir(exist_ok=True)

    run(["g++", "src/Astar.cpp", "-std=c++17", "-O2", "-o", "basic_astar"], root)
    run(["g++", "src/AdvancedAstar.cpp", "-std=c++17", "-O2", "-o", "advanced_astar"], root)

    for input_path in sorted((root / "input").glob("input_*.txt")):
        out_name = input_path.name.replace("input_", "output_")
        run(["./basic_astar", str(input_path.relative_to(root)), str(Path("base_output") / out_name)], root)
        run(["./advanced_astar", str(input_path.relative_to(root)), str(Path("advanced_output") / out_name)], root)

    run(["python", "tools/evaluate_base.py", "--input", "input", "--output", "base_output"], root)
    run(["python", "tools/evaluate_advanced.py", "--input", "input", "--base_output", "base_output", "--advanced_output", "advanced_output"], root)
    run(["python", "tools/visualize.py", "--input", "input/input_0.txt", "--base", "base_output/output_0.txt", "--advanced", "advanced_output/output_0.txt", "--save", "vis_0.png"], root)


if __name__ == "__main__":
    main()
