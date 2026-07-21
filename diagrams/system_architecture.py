"""
N4 Base Station - System Architecture Diagram
Shows start_basestation.py and all managed services.
Generates: output/system_architecture.png
"""
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch
import os

os.makedirs(os.path.join(os.path.dirname(os.path.abspath(__file__)), "output"), exist_ok=True)
OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "output", "system_architecture.png")

BG           = "#0d1117"
ENTRY_COLOR  = "#3b2a6e"
GROUP_COLOR  = "#1c3a5e"
SERVICE_COLOR= "#1a3a2a"
LIFECYCLE_COLOR = "#5c1f1f"
BORDER       = "#58a6ff"
TEXT_MAIN    = "#e6edf3"
TEXT_DIM     = "#8b949e"
PORT_COLOR   = "#f78166"

fig, ax = plt.subplots(figsize=(14, 10))
ax.set_xlim(0, 14)
ax.set_ylim(0, 10)
ax.axis("off")
fig.patch.set_facecolor(BG)
ax.set_facecolor(BG)

def box(x, y, w, h, color, title, sub=None, bc=None):
    bc = bc or BORDER
    p = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.07",
                        facecolor=color, edgecolor=bc, linewidth=1.6, zorder=3)
    ax.add_patch(p)
    ty = y + h/2 + (0.12 if sub else 0)
    ax.text(x+w/2, ty, title, ha="center", va="center",
            color=TEXT_MAIN, fontsize=9.5, fontweight="bold", zorder=4)
    if sub:
        ax.text(x+w/2, y+h/2-0.20, sub, ha="center", va="center",
                color=TEXT_DIM, fontsize=8, zorder=4)

def port_badge(x, y, port):
    p = FancyBboxPatch((x, y), 1.0, 0.30, boxstyle="round,pad=0.04",
                        facecolor="#1c1c2e", edgecolor=PORT_COLOR, linewidth=1.0, zorder=4)
    ax.add_patch(p)
    ax.text(x+0.50, y+0.15, port, ha="center", va="center",
            color=PORT_COLOR, fontsize=7.5, fontweight="bold", zorder=5)

def arrow_v(x, y0, y1):
    ax.annotate("", xy=(x, y1), xytext=(x, y0),
                arrowprops=dict(arrowstyle="-|>", color=BORDER, lw=1.8, mutation_scale=12), zorder=2)

def arrow_h(x0, x1, y):
    ax.annotate("", xy=(x1, y), xytext=(x0, y),
                arrowprops=dict(arrowstyle="-|>", color=BORDER, lw=1.8, mutation_scale=12), zorder=2)

# Title
ax.text(7.0, 9.6, "N4 Base Station — System Architecture",
        ha="center", va="center", color=BORDER, fontsize=14, fontweight="bold", zorder=4)

# Entry point
box(4.5, 8.3, 5.0, 0.85, ENTRY_COLOR, "start_basestation.py", "Main entry point — manages all services")

# Three columns of services
# Column 1: Service Management
box(0.3, 6.4, 3.2, 0.7, GROUP_COLOR, "Mosquitto MQTT Broker")
port_badge(3.55, 6.5, ":1883")
box(0.3, 5.5, 3.2, 0.7, GROUP_COLOR, "Tileserver-GL")
port_badge(3.55, 5.6, ":8080")
box(0.3, 4.6, 3.2, 0.7, GROUP_COLOR, "Vite Dev Server")
port_badge(3.55, 4.7, ":5173")
box(0.3, 3.7, 3.2, 0.7, GROUP_COLOR, "Node.js API Server")
port_badge(3.55, 3.8, ":3000")

# Column 2: Telemetry Server
box(4.7, 6.4, 4.6, 0.7, SERVICE_COLOR, "Serial / Bluetooth Comm")
box(4.7, 5.5, 4.6, 0.7, SERVICE_COLOR, "USB Auto-Reconnect Monitor")
box(4.7, 4.6, 4.6, 0.7, SERVICE_COLOR, "MQTT Bridge  (serial <-> MQTT)")
box(4.7, 3.7, 4.6, 0.7, SERVICE_COLOR, "CSV Logger  +  Telemetry Parser")
box(4.7, 2.8, 4.6, 0.7, SERVICE_COLOR, "Command Processor")

# Column 3: Lifecycle
box(10.3, 6.4, 3.2, 0.7, LIFECYCLE_COLOR, "Port Cleanup")
box(10.3, 5.5, 3.2, 0.7, LIFECYCLE_COLOR, "Graceful Shutdown")
box(10.3, 4.6, 3.2, 0.7, LIFECYCLE_COLOR, "Resource Cleanup")

# Group labels
for gy, lbl, lx in [(6.55, "Service Management", 1.9),
                    (5.15, "Telemetry Server (integrated)", 7.0),
                    (5.70, "Process Lifecycle", 11.9)]:
    ax.text(lx, gy + 0.80, lbl, ha="center", va="center",
            color=TEXT_DIM, fontsize=8, style="italic", zorder=4)

# Arrows from entry point down into three columns
for cx in [1.85, 7.00, 11.90]:
    arrow_v(cx, 8.30, 7.15)

# Legend
legend_items = [(ENTRY_COLOR, "Entry point"), (GROUP_COLOR, "External service"),
                (SERVICE_COLOR, "Integrated module"), (LIFECYCLE_COLOR, "Lifecycle manager")]
lx, ly = 0.25, 2.5
for fc, lbl in legend_items:
    p = FancyBboxPatch((lx, ly-0.14), 0.26, 0.28, boxstyle="round,pad=0.03",
                        facecolor=fc, edgecolor=BORDER, linewidth=1.1, zorder=4)
    ax.add_patch(p)
    ax.text(lx+0.40, ly, lbl, ha="left", va="center", color=TEXT_DIM, fontsize=8, zorder=4)
    ly -= 0.42

plt.tight_layout(pad=0.3)
plt.savefig(OUTPUT, dpi=200, bbox_inches="tight", facecolor=BG)
print(f"Saved  {OUTPUT}")
plt.close()

