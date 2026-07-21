@echo off
echo Starting N4 Base Station (orchestrated)...
echo.
echo This will start:
echo - Mosquitto MQTT broker
echo - Tileserver-GL on port 8080 (offline maps)
echo - Vite dev server (React app)
echo - Python server (headless)
echo.
echo Tips:
echo   - Set N4_TILES_MODE=0 to use Docker tileserver (recommended on Node 22+)
echo   - Set N4_TILES_MODE=1 to use CLI tileserver-gl (requires Node 18/20)
echo   - Optionally set N4_MBTILES to your .mbtiles file name
echo.

python start_basestation.py

echo.
echo Base station stopped.
pause
