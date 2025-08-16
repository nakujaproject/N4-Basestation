#!/usr/bin/env python3
"""
N4 Base Station Startup Script
Starts all required services for the base station
"""

import subprocess
import sys
import os
import signal
import time

def start_basestation():
    """Start the base station with all services"""
    print("🚀 Starting N4 Base Station...")
    print()
    print("This will start:")
    print("- Python server with auto USB reconnection")
    print("- npm dev server (React app)")
    print("- Tileserver for maps")
    print("- Mosquitto MQTT broker")
    print()
    print("Press Ctrl+C to stop all services")
    print()
    
    try:
        # Change to the script directory
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        
        # Start the main server (which will start all other services)
        subprocess.run([sys.executable, "server.py"])
        
    except KeyboardInterrupt:
        print("\n🛑 Shutting down base station...")
    except Exception as e:
        print(f"❌ Error starting base station: {e}")
    finally:
        print("👋 Base station stopped.")

if __name__ == "__main__":
    start_basestation()
