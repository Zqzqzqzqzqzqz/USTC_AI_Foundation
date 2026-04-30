from __future__ import annotations

import argparse
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import numpy as np
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
except ModuleNotFoundError as exc:
    raise SystemExit("visualize.py requires numpy and matplotlib. Install them with: python -m pip install numpy matplotlib") from exc

from common import parse_output, read_scene


def cuboid_faces(x1, y1, z1, x2, y2, z2):
    x2, y2, z2 = x2 + 1, y2 + 1, z2 + 1
    pts = np.array([[x1, y1, z1], [x2, y1, z1], [x2, y2, z1], [x1, y2, z1],
                    [x1, y1, z2], [x2, y1, z2], [x2, y2, z2], [x1, y2, z2]])
    return [[pts[i] for i in face] for face in [(0, 1, 2, 3), (4, 5, 6, 7), (0, 1, 5, 4), (2, 3, 7, 6), (1, 2, 6, 5), (0, 3, 7, 4)]]


def draw_path(ax, points, color, label):
    if not points:
        return
    arr = np.array(points)
    ax.plot(arr[:, 0], arr[:, 1], arr[:, 2], color=color, linewidth=2.5, marker="o", markersize=3, label=label)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--base", type=Path)
    parser.add_argument("--advanced", type=Path)
    parser.add_argument("--save", type=Path)
    parser.add_argument("--show", action="store_true")
    args = parser.parse_args()

    scene = read_scene(args.input)
    fig = plt.figure(figsize=(10, 7))
    ax = fig.add_subplot(111, projection="3d")
    x, y, z = scene["size"]
    ax.set_xlim(0, x)
    ax.set_ylim(0, y)
    ax.set_zlim(0, z)
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title("3D Occupancy Grid Path Planning")

    xx, yy = np.meshgrid([0, x], [0, y])
    zz = np.zeros_like(xx)
    ax.plot_surface(xx, yy, zz, alpha=0.16, color="#9ca3af")

    colors = {"box": "#ef4444", "bottle": "#22c55e", "monitor": "#3b82f6", "cup": "#f59e0b", "tool": "#8b5cf6", "bin": "#64748b"}
    for obs in scene["obstacles"]:
        x1, y1, z1 = obs["min"]
        x2, y2, z2 = obs["max"]
        faces = cuboid_faces(x1, y1, z1, x2, y2, z2)
        poly = Poly3DCollection(faces, alpha=0.42, facecolor=colors.get(obs["label"], "#94a3b8"), edgecolor="#111827", linewidth=0.5)
        ax.add_collection3d(poly)
        ax.text((x1 + x2 + 1) / 2, (y1 + y2 + 1) / 2, z2 + 1.1, obs["label"], fontsize=8)

    sx, sy, sz = scene["start"]
    gx, gy, gz = scene["goal"]
    ax.scatter([sx], [sy], [sz], color="#16a34a", s=70, label="start", depthshade=False)
    ax.scatter([gx], [gy], [gz], color="#dc2626", s=70, label="goal", depthshade=False)

    if args.base and args.base.exists():
        draw_path(ax, parse_output(args.base)["points"], "#2563eb", "base path")
    if args.advanced and args.advanced.exists():
        draw_path(ax, parse_output(args.advanced)["points"], "#f97316", "advanced path")

    ax.legend(loc="upper left")
    ax.view_init(elev=24, azim=-58)
    plt.tight_layout()
    if args.save:
        fig.savefig(args.save, dpi=180)
        print(f"saved {args.save}")
    if args.show:
        plt.show()
    plt.close(fig)


if __name__ == "__main__":
    main()
