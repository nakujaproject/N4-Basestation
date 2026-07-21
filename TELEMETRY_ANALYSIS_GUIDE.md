# 📊 Telemetry Analysis Notebook Guide

## Overview

[Telemetry_Analysis.ipynb](Telemetry_Analysis.ipynb) is a comprehensive Jupyter notebook for analyzing N4 rocket telemetry data after flights or tests.

## Installation

### 1. Install Dependencies

```bash
# Option A: Use requirements file (recommended)
pip install -r analysis_requirements.txt

# Option B: Install manually
pip install jupyter pandas numpy matplotlib seaborn plotly ipywidgets
```

### 2. Launch Notebook

```bash
# Windows PowerShell/CMD
jupyter notebook Telemetry_Analysis.ipynb

# Should automatically open in your browser at http://localhost:8888
```

---

## Usage Workflow

### Step 1: Load Data

**Option A - Select Existing File:**
1. Run Cell 1-4 to initialize and scan for CSV files
2. Cell 4 will display a dropdown of available telemetry files
3. Select your file from the dropdown
4. Click "Load Selected File" button

**Option B - Upload Custom CSV:**
1. Run cells 1-4
2. Use the file upload widget in Cell 5
3. Click "Upload CSV" and select your file

### Step 2: Run Analysis

Execute cells sequentially (Shift+Enter or click "Run"):

- **Cell 6**: Data loading and validation
- **Cell 7**: Data quality report
- **Cell 8**: Flight phase detection
- **Cell 9**: Altitude profile plots
- **Cell 10**: Acceleration analysis
- **Cell 11**: Battery/system health
- **Cell 12**: GPS trajectory (if available)
- **Cell 13**: Flight summary report
- **Cell 14**: Export processed data

### Step 3: Review Results

Each analysis cell produces:
- **Text Output**: Statistics, phase times, performance metrics
- **Interactive Plots**: Zoom, pan, hover for details
- **Phase Markers**: Vertical lines indicating launch/apogee/landing

### Step 4: Export (Optional)

Cell 14 exports:
- `analysis_exports/processed_telemetry_YYYYMMDD_HHMMSS.csv` - Cleaned data
- `analysis_exports/statistics_YYYYMMDD_HHMMSS.csv` - Summary statistics
- `analysis_exports/flight_phases_YYYYMMDD_HHMMSS.json` - Phase markers
- `flight_report_YYYYMMDD_HHMMSS.txt` - Text summary

---

## Analysis Features

### 1. Data Quality Report

- **Total Records**: Number of telemetry data points
- **Time Span**: Flight duration in seconds/minutes
- **Update Rate**: Average telemetry frequency (Hz)
- **Missing Data**: Percentage of missing values per column
- **Value Ranges**: Min/max/mean for key telemetry

**Example Output:**
```
📊 DATA QUALITY REPORT
==============================================================

📈 Dataset Overview:
   Total Records: 8,432
   Time Span: 156.4 seconds (2.6 minutes)
   Average Update Rate: 53.9 Hz

🔍 Data Completeness:
   gps_altitude        ████████████████░░░░ 85.3% missing (7,192 values)
   wifi_rssi           ██████████████░░░░░░ 72.1% missing (6,079 values)
   latitude            ████████████░░░░░░░░ 68.4% missing (5,767 values)
   ...

📏 Value Ranges:
   agl_altitude          Min:     0.00  Max:   834.20  Mean:   287.45
   velocity              Min:    -15.30  Max:   125.80  Mean:    42.10
   battery_voltage       Min:     3.72  Max:     4.15  Mean:     3.94
```

### 2. Flight Phase Detection

Automatically identifies:
- **Launch**: Altitude > 10m
- **Apogee**: Maximum altitude point
- **Landing**: Altitude returns to ~0m

**Example Output:**
```
🚀 FLIGHT PHASE DETECTION
==============================================================

🛫 Launch Detected:
   Time: T+2.3s
   Record #: 125

🎯 Apogee Reached:
   Altitude: 834.2m
   Time: T+24.8s
   Record #: 1,338

🛬 Landing Detected:
   Time: T+156.4s
   Record #: 8,425

⏱️ Total Flight Time: 154.1s (2.6 minutes)
```

### 3. Altitude Profile

Interactive plot showing:
- Kalman filtered altitude (blue line)
- Raw barometric altitude (light blue line)
- Vertical velocity (purple line, bottom panel)
- Phase markers (green=launch, red=apogee, orange=landing)

**Features:**
- Hover to see exact values
- Zoom/pan with mouse
- Toggle traces on/off (click legend)

### 4. Acceleration Analysis

3-axis acceleration (X, Y, Z) over time:
- **Axis X**: Red line (lateral acceleration)
- **Axis Y**: Green line (lateral acceleration)
- **Axis Z**: Blue line (vertical acceleration)

**Statistics:**
```
📊 Acceleration Statistics:
   AX: Max = 2.34g, Min = -1.87g, Mean = 0.12g
   AY: Max = 3.12g, Min = -2.45g, Mean = 0.08g
   AZ: Max = 12.45g, Min = -2.10g, Mean = 1.02g
```

### 5. Battery & System Health

Monitors:
- **Battery Voltage**: Track discharge over flight
- **Critical Level Warning**: Red line at 3.3V
- **RSSI**: Signal strength (dBm)

**Example:**
```
🔋 Battery Analysis:
   Start Voltage: 4.15V
   End Voltage: 3.89V
   Voltage Drop: 0.26V
   Minimum: 3.87V
```

### 6. GPS Trajectory

Interactive map showing:
- Flight path with color-coded time
- Launch and landing coordinates
- Horizontal distance traveled

**Example:**
```
📍 GPS Statistics:
   Valid GPS Points: 432
   Launch Coordinates: -1.234567, 36.789012
   Landing Coordinates: -1.234890, 36.789345
   Horizontal Distance: 487.3m
```

### 7. Flight Summary Report

Comprehensive text report including:
- Dataset information
- Flight phases (times and altitudes)
- Performance metrics (max velocity, max acceleration)
- Battery status
- Saved to: `flight_report_YYYYMMDD_HHMMSS.txt`

---

## Supported CSV Formats

### Base Station Format (28 fields)

Generated by `start_basestation.py` with CSV logging enabled.

**Columns:**
```csv
timestamp,iso_timestamp,record_number,operation_mode,state,
ax,ay,az,pitch,roll,gx,gy,gz,
latitude,longitude,gps_altitude,gps_time,
pressure,temperature,agl_altitude,velocity,
pyro1_state,pyro2_state,battery_voltage,wifi_rssi,
kalman_altitude,kalman_vertical_velocity,
communication_mode,raw_data
```

**Key Fields:**
- `agl_altitude`: Altitude above ground level (barometer)
- `kalman_altitude`: Filtered altitude (Kalman filter)
- `kalman_vertical_velocity`: Filtered velocity
- `ax/ay/az`: Acceleration (g)
- `gx/gy/gz`: Gyroscope (deg/s)
- `battery_voltage`: Battery voltage (V)
- `state`: Flight state (0-6)

### XBee UART Format (6 fields)

Generated by [uart_csv_receiver_ground.ino](research/xbee/code_examples/uart_production/uart_csv_receiver_ground/uart_csv_receiver_ground.ino).

**Columns:**
```csv
timestamp,state,altitude,velocity,accel_z,battery
```

**Note:** Simplified format with essential flight data only. The notebook auto-detects this format and adjusts parsing accordingly.

---

## Troubleshooting

### Issue: "No module named 'pandas'"

**Solution:**
```bash
pip install -r analysis_requirements.txt
```

### Issue: "No telemetry CSV files found"

**Solution:**
- Check that `telemetry_logs/` directory exists
- Verify CSV files are present: `dir telemetry_logs\*.csv`
- Use the upload widget (Option B) to upload CSV from another location

### Issue: Plots not showing

**Solution:**
1. Ensure you ran all cells in order
2. Check that `data_loaded = True` (printed after loading)
3. Try restarting kernel: Kernel → Restart & Run All

### Issue: "KeyError: 'kalman_altitude'"

**Solution:**
- CSV format may be missing expected columns
- Notebook will attempt to use alternative columns (e.g., `agl_altitude`, `gps_altitude`)
- Check Cell 3 output for available columns

### Issue: GPS map not showing

**Solution:**
- Requires `latitude` and `longitude` columns with valid data
- If GPS data is missing/incomplete, map will not render
- Check data quality report for GPS completeness

### Issue: Jupyter not opening in browser

**Solution:**
```bash
# Get Jupyter URL with token
jupyter notebook list

# Copy URL and paste in browser manually
# Example: http://localhost:8888/?token=abc123def456...
```

---

## Keyboard Shortcuts (Jupyter)

- **Shift+Enter**: Run current cell and move to next
- **Ctrl+Enter**: Run current cell (stay on same cell)
- **Alt+Enter**: Run current cell and insert new cell below
- **A**: Insert cell above
- **B**: Insert cell below
- **D,D**: Delete current cell
- **M**: Convert cell to Markdown
- **Y**: Convert cell to Code
- **Esc**: Exit edit mode
- **Enter**: Enter edit mode

---

## Custom Analysis (Cell 12)

Use this cell to add your own analysis code:

```python
# Example: Compare Kalman vs raw altitude
if data_loaded and df is not None:
    fig = px.line(df, x='time_elapsed', y=['kalman_altitude', 'agl_altitude'],
                  title='Kalman Filter Performance',
                  labels={'value': 'Altitude (m)', 'time_elapsed': 'Time (s)'})
    fig.show()
```

**Access DataFrame:**
- `df` - Full telemetry data
- `phases` - Flight phase markers dictionary

**Example Analyses:**
- Gyroscope rate analysis
- Temperature vs altitude correlation
- RSSI signal strength analysis
- State transition timing
- Pyro channel activation verification

---

## Tips & Best Practices

1. **Run Cells in Order**: Always run from top to bottom on first execution
2. **Check Data Quality**: Review Cell 7 output before interpreting plots
3. **Interactive Plots**: Use zoom/pan tools in Plotly plots for detailed inspection
4. **Export Early**: Run Cell 14 to save results before closing notebook
5. **Multiple Files**: Re-run cells 4-14 to analyze different files without restarting kernel
6. **Custom Analysis**: Add your own cells at the end for specific investigations
7. **Save Regularly**: Ctrl+S to save notebook (preserves outputs)

---

## Next Steps

- **Compare Flights**: Load multiple CSV files and create comparison plots
- **Statistical Analysis**: Add cells for correlation analysis, outlier detection
- **Machine Learning**: Train models to predict apogee or detect anomalies
- **Automated Reports**: Export professional PDF reports with matplotlib/reportlab
- **Real-time Analysis**: Integrate with base station for live analysis

---

## Support

**Documentation:**
- Main README: [README.md](README.md)
- Research Directory: [research/README.md](research/README.md)
- XBee Setup: [research/xbee/README.md](research/xbee/README.md)

**Contact:**
- GitHub Issues: [nakujaproject/n4-basestation](https://github.com/nakujaproject/n4-basestation/issues)
- Nakuja Project: recovery-team@nakujaproject.org

---

**Last Updated:** January 17, 2026  
**Version:** 1.0.0  
**Notebook:** [Telemetry_Analysis.ipynb](Telemetry_Analysis.ipynb)
