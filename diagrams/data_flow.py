"""
N4 Base Station - Data Flow Diagram
Shows telemetry data path: Flight Computer -> Base Station -> Dashboard.
Generates: output/data_flow.png
"""
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import os

OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "output", "data_flow.png")
os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)

BG         = "#0d1117"
C_ESP      = "#1c3a5e"
C_PROTO    = "#3b2a6e"
C_PY       = "#1a3a2a"
C_MQTT     = "#5c3a1f"
C_DASH     = "#1a2a3c"
BORDER     = "#58a6ff"
TEXT_MAIN  = "#e6edf3"
TEXT_DIM   = "#8b949e"
TEXT_PORT  = "#f78166"
ARROW_CLR  = "#58a6ff"

fig, ax = plt.subplots(figsize=(7, 13))
ax.set_xlim(0, 7)
ax.set_ylim(0, 13)
ax.axis("off")
fig.patch.set_facecolor(BG)
ax.set_facecolor(BG)

def box(x, y, w, h, color, title, sub=None):
    p = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.1",
                        facecolor=color, edgecolor=BORDER, linewidth=1.8, zorder=3)
    ax.add_patch(p)
    ty = y + h / 2 + (0.15 if sub else 0)
    ax.text(x + w / 2, ty, title, ha="center", va="center",
            color=TEXT_MAIN, fontsize=10.5, fontweight="bold", zorder=4)
    if sub:
        ax.text(x + w / 2, y + h / 2 - 0.22, sub,
                ha="center", va="center", color=TEXT_DIM, fontsize=8.5, zorder=4)

def label_arrow(x, y0, y1, lbl, sublbl=None):
    ax.annotate("", xy=(x, y1), xytext=(x, y0),
                arrowprops=dict(arrowstyle="-|>", color=ARROW_CLR, lw=2.2,
                                mutation_scale=16), zorder=2)
    mx = x + 0.5
    my = (y0 + y1) / 2
    ax.text(mx, my + (0.12 if sublbl else 0), lbl,
            ha="left", va="center", color=TEXT_DIM, fontsize=8.5, zorder=4)
    if sublbl:
        ax.text(mx, my - 0.16, sublbl, ha="left", va="center",
                color=TEXT_PORT, fontsize=7.5, zorder=4)

cx = 2.0    # center x of all boxes
bw = 3.0    # box width

# Title
ax.text(3.5, 12.5, "N4 Base Station — Telemetry Data Flow",
        ha="center", va="center", color=BORDER, fontsize=13, fontweight="bold", zorder=4)

# Nodes top → bottom
nodes = [
    (11.3, C_ESP,   "Flight Computer (ESP32)",      "Sensor acquisition & state machine"),
    ( 9.5, C_PROTO, "ESP-NOW RF Link",               "2.4 GHz encrypted broadcast"),
    ( 7.7, C_ESP,   "Base Station ESP32",             "RSSI + packet relay"),
    ( 5.9, C_PROTO, "Serial / Bluetooth (COM port)",  "115 200 baud"),
    ( 4.1, C_PY,    "start_basestation.py",           "Telemetry parser + MQTT bridge"),
    ( 2.3, C_MQTT,  "MQTT Broker  (Mosquitto)",       "Topic: n4/app/flight-computer-1   :1883"),
    ( 0.5, C_DASH,  "React Dashboard",                "localhost:5173  —  live telemetry"),
]

for y, color, title, sub in nodes:
    box(cx, y, bw, 0.9, color, title, sub)

# Arrows between consecutive nodes
arrows = [
    (11.3, 10.4, "ESP-NOW", "encrypted 2.4 GHz"),
    ( 9.5,  8.6, "decoded packet", "RSSI + metadata"),
    ( 7.7,  6.8, "serial frame", "USB or BT"),
    ( 5.9,  5.0, "parsed CSV / JSON", None),
    ( 4.1,  3.2, "MQTT publish", "QoS 0"),
    ( 2.3,  1.4, "WebSocket / SSE", "real-time update"),
]

ax_x = cx + bw / 2
for y0, y1, lbl, sub in arrows:
    label_arrow(ax_x, y0, y1, "  " + lbl, sub)

plt.tight_layout(pad=0.4)
plt.savefig(OUTPUT, dpi=200, bbox_inches="tight", facecolor=BG)
print(f"Saved  {OUTPUT}")
plt.close()
