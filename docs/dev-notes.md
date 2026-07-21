# Dev notes: Vite HMR and services (Windows)

If the browser shows `WebSocket opening handshake timed out` from `ws://localhost:5173`, try:

- Ensure only one Vite server is running (port 5173). Close duplicates.
- Use 127.0.0.1 in the URL (not `localhost`) to avoid IPv6/hosts quirks.
- We pin Vite dev server to host 127.0.0.1 and `hmr.host=127.0.0.1` in `vite.config.js`.
- Disable VPN/antivirus that might intercept websockets.

Services:

- Tiles: Python MBTiles server on port 8080 using `osm-2020-02-10-v3.11_africa_kenya.mbtiles` (no Docker, no tileserver-gl).
- MQTT: `mosquitto -c mosquitto.conf -v`.
- Orchestrator: run `python server.py` (non-headless) to auto-start services; set `N4_HEADLESS=1` to skip.
