import json
import sys
import pandas as pd

rows = [json.loads(l) for l in open("jetson64_sweep_results.jsonl")]
df = pd.json_normalize(rows)

# Generation 0 is cold: caches, clock ramp, population still settling.
df = df[df["gen"] > 0].copy()

# Normalising for the drifting population is the point of this column.
df["us_per_agent"]   = df["us_total"] / df["agents"].clip(lower=1)
df["sense_frac"]     = df["us_sense"] / df["us_total"].clip(lower=1e-9)
df["temp_rise"]      = df["platform_after.temp_max_c"] - df["platform_before.temp_max_c"]

# Hardware-normalised throughput, for comparing across devices.
df["tps_per_sm_ghz"] = (df["ticks_per_s"] /
                        (df["header.sms"] * df["header.clock_khz"] / 1e6))

AXIS = {
    "lattice_cell":   "config.LATTICE_CELL_WIDTH",
    "world_count":    "config.worlds",
    "population":     "config.prey",
    "const_density":  "config.NUM_CELLE_X",
    "const_count":    "config.NUM_CELLE_X",
    "sensing_radius": "config.grass_radius",
    "food_refresh":   "config.FOOD_CHUNK_REFRESH_INTERVAL",
    "grid_mult":      "config.grid_mult",
    "brain_size":     "config.HIDDEN_LAYER_SIZE",
}

for sweep in sorted(df["config.sweep"].dropna().unique()):
    axis = AXIS.get(sweep)
    if axis is None or axis not in df.columns:
        continue
    sub = df[df["config.sweep"] == sweep]
    print(f"\n=== {sweep} (median over seeds) ===")
    print(sub.groupby(axis)[
        ["ticks_per_s", "us_total", "us_per_agent", "sense_frac", "agents"]
    ].median().round(2))

hot = df[df["temp_rise"] > 5]
if len(hot):
    print(f"\nWARNING: {len(hot)} runs heated the board by more than 5 C. "
          "Thermal drift may be contaminating those points.")