#!/usr/bin/env python3
"""
Simple CSV Telemetry Viewer
Real-time viewer for telemetry CSV logs
"""

import csv
import time
import os
from pathlib import Path
from datetime import datetime

def get_latest_csv():
    """Get the most recent CSV file"""
    log_dir = Path("telemetry_logs")
    if not log_dir.exists():
        return None
    
    csv_files = list(log_dir.glob("telemetry_*.csv"))
    if not csv_files:
        return None
        
    return max(csv_files, key=lambda f: f.stat().st_mtime)

def tail_csv_file(csv_file, lines=10):
    """Show the last N lines of a CSV file"""
    try:
        with open(csv_file, 'r', newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            
            if not rows:
                print("📄 CSV file is empty")
                return
            
            print(f"\n📊 Last {min(lines, len(rows))} records from: {csv_file.name}")
            print("=" * 120)
            
            # Show header info
            print(f"{'Time':<20} {'Record':<8} {'State':<6} {'Alt(m)':<8} {'Vel(m/s)':<10} {'RSSI':<6} {'Mode':<8}")
            print("-" * 120)
            
            for row in rows[-lines:]:
                timestamp = row.get('iso_timestamp', '')[:19]  # Remove microseconds
                record_num = row.get('record_number', 'N/A')
                state = row.get('state', 'N/A')
                altitude = row.get('agl_altitude', 'N/A')
                velocity = row.get('velocity', 'N/A')
                rssi = row.get('wifi_rssi', 'N/A')
                comm_mode = row.get('communication_mode', 'N/A')
                
                # Format altitude and velocity
                try:
                    if altitude != 'N/A' and altitude != '':
                        altitude = f"{float(altitude):.2f}"
                except:
                    pass
                    
                try:
                    if velocity != 'N/A' and velocity != '':
                        velocity = f"{float(velocity):.2f}"
                except:
                    pass
                
                print(f"{timestamp:<20} {record_num:<8} {state:<6} {altitude:<8} {velocity:<10} {rssi:<6} {comm_mode:<8}")
                
    except Exception as e:
        print(f"❌ Error reading CSV: {e}")

def monitor_csv(csv_file, refresh_interval=2):
    """Monitor CSV file for new data"""
    print(f"🔍 Monitoring: {csv_file}")
    print("Press Ctrl+C to stop")
    
    last_size = 0
    
    try:
        while True:
            current_size = os.path.getsize(csv_file)
            
            if current_size > last_size:
                # File has grown, show new data
                os.system('cls' if os.name == 'nt' else 'clear')  # Clear screen
                print(f"🔄 Updated: {datetime.now().strftime('%H:%M:%S')}")
                tail_csv_file(csv_file, 15)
                last_size = current_size
            
            time.sleep(refresh_interval)
            
    except KeyboardInterrupt:
        print("\n👋 Monitoring stopped")
    except Exception as e:
        print(f"❌ Monitoring error: {e}")

def main():
    """Main function"""
    csv_file = get_latest_csv()
    
    if not csv_file:
        print("❌ No CSV log files found in telemetry_logs/")
        print("💡 Make sure the base station is running and logging data")
        return
    
    print("🚀 N4 Telemetry CSV Viewer")
    print("=" * 50)
    
    while True:
        try:
            print(f"\nCurrent file: {csv_file.name}")
            print("\nOptions:")
            print("1. Show last 10 records")
            print("2. Show last 25 records")
            print("3. Monitor file (real-time)")
            print("4. Refresh file list")
            print("5. Exit")
            
            choice = input("\nEnter choice (1-5): ").strip()
            
            if choice == '1':
                tail_csv_file(csv_file, 10)
            elif choice == '2':
                tail_csv_file(csv_file, 25)
            elif choice == '3':
                monitor_csv(csv_file)
            elif choice == '4':
                new_file = get_latest_csv()
                if new_file:
                    csv_file = new_file
                    print(f"✅ Updated to: {csv_file.name}")
                else:
                    print("❌ No CSV files found")
            elif choice == '5':
                break
            else:
                print("❌ Invalid choice")
                
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"❌ Error: {e}")
    
    print("👋 Goodbye!")

if __name__ == "__main__":
    main()
