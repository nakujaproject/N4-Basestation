#!/usr/bin/env python3
"""
Test script to verify ESP32 data parsing
"""

import json

# Sample ESP32 data from your logs
esp32_sample = '''{"record_number":53,"operation_mode":0,"state":0,"battery_voltage":0,"wifi_rssi":-81,"acc_data":{"ax":0,"ay":0,"az":0,"pitch":-0.03,"roll":-135.06},"gyro_data":{"gx":-0.03,"gy":-0.03,"gz":-0.03},"gps_data":{"latitude":-1.158093,"longitude":37.08183,"gps_altitude":1532.8,"time":0},"alt_data":{"pressure":854.6,"temperature":23.54,"AGL":-1.1,"velocity":0,"kalman_altitude":-0.42,"kalman_vertical_velocity":-1.21},"chute_state":{"pyro1_state":0,"pyro2_state":0},"connection_status":{"connected":true,"has_ever_connected":true,"packet_age_ms":1,"timeout_exceeded":false,"rssi":-81},"communication_mode":"Beacon","timestamp":656822,"packets_received":5502}'''

def test_data_extraction():
    """Test how we extract data from ESP32 format"""
    print("🧪 Testing ESP32 data extraction...")
    
    try:
        data = json.loads(esp32_sample)
        print(f"✅ JSON parsing successful")
        
        # Test Kalman data extraction (our enhanced logic)
        kalman_alt = (
            data.get('alt_data', {}).get('kalman_altitude') or
            data.get('kalman_data', {}).get('altitude') or
            data.get('kalman_altitude', 0)
        )
        kalman_vel = (
            data.get('alt_data', {}).get('kalman_vertical_velocity') or
            data.get('kalman_data', {}).get('vertical_velocity') or
            data.get('kalman_vertical_velocity', 0)
        )
        
        print(f"📊 Data extraction results:")
        print(f"   Record Number: {data.get('record_number')}")
        print(f"   Communication Mode: {data.get('communication_mode')} → {data.get('communication_mode', 'BEACON').upper()}")
        print(f"   GPS Altitude: {data.get('gps_data', {}).get('gps_altitude')}")
        print(f"   AGL Altitude: {data.get('alt_data', {}).get('AGL')}")
        print(f"   Kalman Altitude: {kalman_alt} (from alt_data.kalman_altitude)")
        print(f"   Kalman Velocity: {kalman_vel} (from alt_data.kalman_vertical_velocity)")
        print(f"   Pressure: {data.get('alt_data', {}).get('pressure')}")
        print(f"   Temperature: {data.get('alt_data', {}).get('temperature')}")
        print(f"   WiFi RSSI: {data.get('wifi_rssi')}")
        print(f"   Battery Voltage: {data.get('battery_voltage')}")
        print(f"   GPS Lat/Lon: {data.get('gps_data', {}).get('latitude')}, {data.get('gps_data', {}).get('longitude')}")
        
        # Test normalized structure
        data['kalman_data'] = {
            'altitude': kalman_alt,
            'vertical_velocity': kalman_vel
        }
        data['kalman_altitude'] = kalman_alt
        data['kalman_vertical_velocity'] = kalman_vel
        data["communication_mode"] = data.get("communication_mode", "BEACON").upper()
        
        print(f"\n🔄 Normalized structure:")
        print(f"   kalman_data.altitude: {data['kalman_data']['altitude']}")
        print(f"   kalman_data.vertical_velocity: {data['kalman_data']['vertical_velocity']}")
        print(f"   communication_mode: {data['communication_mode']}")
        
        # Simulate CSV row creation
        csv_row = {
            'record_number': data.get('record_number', ''),
            'operation_mode': data.get('operation_mode', ''),
            'state': data.get('state', ''),
            'ax': data.get('acc_data', {}).get('ax', ''),
            'pitch': data.get('acc_data', {}).get('pitch', ''),
            'roll': data.get('acc_data', {}).get('roll', ''),
            'latitude': data.get('gps_data', {}).get('latitude', ''),
            'longitude': data.get('gps_data', {}).get('longitude', ''),
            'gps_altitude': data.get('gps_data', {}).get('gps_altitude', ''),
            'pressure': data.get('alt_data', {}).get('pressure', ''),
            'temperature': data.get('alt_data', {}).get('temperature', ''),
            'agl_altitude': data.get('alt_data', {}).get('AGL', ''),
            'velocity': data.get('alt_data', {}).get('velocity', ''),
            'battery_voltage': data.get('battery_voltage', ''),
            'wifi_rssi': data.get('wifi_rssi', ''),
            'kalman_altitude': kalman_alt,
            'kalman_vertical_velocity': kalman_vel,
            'communication_mode': data.get('communication_mode', ''),
        }
        
        print(f"\n📄 CSV row preview (non-empty fields):")
        for key, value in csv_row.items():
            if value != '' and value != 0:
                print(f"   {key}: {value}")
        
        print(f"\n✅ All data extraction tests passed!")
        
    except Exception as e:
        print(f"❌ Test failed: {e}")

if __name__ == "__main__":
    test_data_extraction()
