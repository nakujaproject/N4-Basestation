import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
csv_path = 'telemetry_logs/telemetry_20250813_144420.csv'
df = pd.read_csv(csv_path)

# RSSI decay plot
plt.figure(figsize=(10,4))
plt.plot(df['record_number'], df['wifi_rssi'], marker='o', linestyle='-', color='tab:blue')
plt.title('Range Test 2: RSSI Decay')
plt.xlabel('Record Number')
plt.ylabel('WiFi RSSI (dBm)')
plt.grid(True)
plt.tight_layout()
plt.savefig('Range_Test_2_RSSI.png')
plt.close()

# GPS path plot
plt.figure(figsize=(8,8))
plt.plot(df['longitude'], df['latitude'], marker='o', linestyle='-', color='tab:green')
plt.title('Range Test 2: GPS Path')
plt.xlabel('Longitude')
plt.ylabel('Latitude')
plt.grid(True)
plt.tight_layout()
plt.savefig('Range_Test_2_GPS.png')
plt.close()

print('Plots saved: Range_Test_2_RSSI.png, Range_Test_2_GPS.png')
