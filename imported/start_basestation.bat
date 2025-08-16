@echo off
echo Starting N4 Base Station...
echo.
echo This will start:
echo - Python server with auto USB reconnection
echo - npm dev server (React app)
echo - Tileserver for maps
echo - Mosquitto MQTT broker
echo.
echo Press Ctrl+C to stop all services
echo.

python server.py

echo.
echo Base station stopped.
pause
