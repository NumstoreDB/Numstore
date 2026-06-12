import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from scipy.interpolate import griddata

if len(sys.argv) != 2:
    print("Usage: python viz.py <data file>")
    sys.exit(1)

HEADER = [
        "Unbuffered Time (ms)",
        "Buffered Time (ms)",
        "Smart Files Time (ms)",
        "Unbuffered Memory (kb)",
        "Buffered Memory (kb)",
        "Smart Files Memory (kb)",
        "File Size (kb)",
        "Offset (kb)",
        "Insert Size (kb)",
        "Chunk Size (kb)",
        ]

with open(sys.argv[1], "r") as f:
    lines = [l for l in f.readlines() if l.strip()]

rows = []
for line in lines:
    values = line.split()
    try:
        row = [float(v) for v in values]
        if len(row) == len(HEADER):
            rows.append(row)
    except ValueError:
        continue

df = pd.DataFrame(rows, columns=HEADER)
print("Rows loaded:", len(df))

TIME_COLS = ["Unbuffered Time (ms)", "Buffered Time (ms)", "Smart Files Time (ms)"]
LABELS    = ["Unbuffered", "Buffered", "Smart Files"]
CMAP      = "inferno"

x_raw = df["File Size (kb)"].values
y_raw = df["Offset (kb)"].values

xi = np.linspace(x_raw.min(), x_raw.max(), 80)
yi = np.linspace(y_raw.min(), y_raw.max(), 80)
XI, YI = np.meshgrid(xi, yi)

# Shared color scale across all three plots
grids = []
for col in TIME_COLS:
    ZI = griddata((x_raw, y_raw), df[col].values, (XI, YI), method="linear")
    ZI = np.clip(ZI, 0, None)
    ZI = np.nan_to_num(ZI, nan=0.0)
    grids.append(ZI)

vmin = min(Z.min() for Z in grids)
vmax = max(Z.max() for Z in grids)

fig, axes = plt.subplots(1, 3, figsize=(20, 6), facecolor="#0e1117")
fig.suptitle("Insert Time  —  File Size × Offset", color="white", fontsize=16, fontweight="bold")

for ax, ZI, label in zip(axes, grids, LABELS):
    ax.set_facecolor("#1a1d27")
    im = ax.contourf(XI, YI, ZI, levels=40, cmap=CMAP, vmin=vmin, vmax=vmax)
    ax.contour(XI, YI, ZI, levels=10, colors="white", linewidths=0.3, alpha=0.25)
    ax.set_title(label, color="white", fontsize=12, pad=8)
    ax.set_xlabel("File Size (kb)", color="#aaaaaa", fontsize=9)
    ax.set_ylabel("Offset (kb)",    color="#aaaaaa", fontsize=9)
    ax.tick_params(colors="#888888", labelsize=8)
    for spine in ax.spines.values():
        spine.set_edgecolor("#333344")

cb = fig.colorbar(
        plt.cm.ScalarMappable(
            norm=plt.Normalize(vmin=vmin, vmax=vmax),
            cmap=CMAP
            ),
        ax=axes.tolist(),
        shrink=0.8,
        pad=0.02
        )
cb.set_label("Time (ms)", color="#aaaaaa", fontsize=10)
cb.set_ticks(np.linspace(vmin, vmax, 6))
cb.set_ticklabels([f"{v:.2f}" for v in np.linspace(vmin, vmax, 6)])
cb.ax.yaxis.set_tick_params(color="#888888", labelsize=8)
cb.outline.set_edgecolor("#333344")
plt.setp(cb.ax.yaxis.get_ticklabels(), color="#aaaaaa")

plt.savefig("benchmark.png", dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor())
plt.show()
