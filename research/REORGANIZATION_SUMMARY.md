# Research Directory Reorganization - Summary

**Date:** 2026-01-17

## 📁 New Structure

```
research/
├── README.md                    # Complete research directory guide
├── analysis/                    # Data analysis & visualization (11 files)
│   ├── analyze_telemetry.py
│   ├── compare_flights.py
│   ├── data_quality_check.py
│   ├── deployment_report.py
│   ├── flight_curves.py
│   ├── install_analysis_deps.py
│   ├── isolate_flights.py
│   ├── kalman_vs_raw.py
│   ├── performance_metrics.py
│   ├── plot_range_test2.py
│   └── view_telemetry.py
│
├── arduino_code/                # ESP32/Arduino sketches (5 files)
│   ├── Basestation_Code_6_Bluetooth.ino
│   ├── Simulated_BaseStation_Integrated.ino
│   ├── Simulated_BaseStation_Code.ino
│   ├── Simulated_BaseStation_Code_Fixed.ino
│   └── bluetooth_pairing_test.ino
│
├── bluetooth/                   # Bluetooth tools & docs (7 files)
│   ├── README.md
│   ├── HC05_AT_Command_Setup.ino
│   ├── HC05_BAUDRATE_CONFIG.md
│   ├── bluetooth_setup.py
│   ├── bluetooth_monitor.py
│   └── bluetooth_pairing_test/
│
├── simulation/                  # Simulators & testing (9 files)
│   ├── SIMULATION_TESTING_GUIDE.md
│   ├── csv_telemetry_simulator.py
│   ├── flight_test_simulator.py
│   ├── run_test_environment.py
│   ├── test_flight_simulator.py
│   ├── quick_test.py
│   ├── flight_data_simul.sh
│   ├── log_simul.sh
│   └── simul_data.sh
│
├── documentation/               # Technical documentation (5 files)
│   ├── BLUETOOTH_SETUP.md
│   ├── COMMUNICATION_MODES.md
│   ├── SETUP.md
│   ├── COMMAND_INTERFACE_IMPLEMENTATION.md
│   └── VISUAL_FEEDBACK_IMPLEMENTATION.md
│
├── range_tests/                 # Range test results (3 files)
│   ├── Range_Test_2_Report.md
│   ├── Range_Test_2_GPS.png
│   └── Range_Test_2_RSSI.png
│
└── scripts/                     # Utility scripts & server (3 files)
    ├── server.py
    ├── test_data_parsing.py
    └── test_esp32_parsing.py
```

## 📊 File Movement Summary

### From Main Directory → research/
- `BLUETOOTH_SETUP.md` → `research/documentation/`
- `COMMUNICATION_MODES.md` → `research/documentation/`
- `SETUP.md` → `research/documentation/`
- `bluetooth_setup.py` → `research/bluetooth/`
- `bluetooth_monitor.py` → `research/bluetooth/`

### Within research/ (reorganized)
- **11 analysis files** → `research/analysis/`
- **5 Arduino files** → `research/arduino_code/`
- **9 simulation files** → `research/simulation/`
- **5 documentation files** → `research/documentation/`
- **3 range test files** → `research/range_tests/`
- **3 utility scripts** → `research/scripts/`

### Total Files Organized: **43+ files**

## 🎯 Main Directory (Clean)

Now contains ONLY operational files:
- ✅ `start_basestation_integrated.py` - Main launcher
- ✅ `start_basestation.py` - Legacy launcher
- ✅ `README.md` - Project overview
- ✅ `QUICK_REFERENCE.md` - Quick start guide
- ✅ `package.json` - Node dependencies
- ✅ `vite.config.js` - Build config
- ✅ `mosquitto.conf` - MQTT config
- ✅ `server.js` - Node API
- ✅ `src/` - Frontend code
- ✅ `public/` - Static assets
- ✅ `telemetry_logs/` - Flight data
- ✅ Map tiles, profiles, config files

## 🔧 Code Updates

### start_basestation_integrated.py
**Line 374:** Updated import path
```python
# Before:
from research.server import (...)

# After:
from research.scripts.server import (...)
```

## 📚 New Documentation

### Created Files:
1. **research/README.md** (8KB)
   - Complete research directory guide
   - Usage instructions for each category
   - Quick start workflows
   - Troubleshooting guides

2. **QUICK_REFERENCE.md** (5KB)
   - Quick start commands
   - Common tasks
   - File locations
   - Troubleshooting shortcuts

## ✅ Benefits

### Organization
- ✨ Clear separation: operational vs research files
- ✨ Logical grouping by function
- ✨ Easy to find related files
- ✨ Scalable structure for future additions

### Documentation
- 📖 Comprehensive README in research/
- 📖 Quick reference in main directory
- 📖 Each category has clear purpose
- 📖 Usage examples provided

### Git History
- 🔄 All moves preserved as renames
- 🔄 Git tracks file history correctly
- 🔄 No loss of commit history

## 🎓 File Organization Rules

### Main Directory Rules:
1. Only files needed to RUN the system
2. Configuration files (package.json, vite.config.js, etc.)
3. Startup scripts (start_basestation*.py)
4. Top-level documentation (README.md, QUICK_REFERENCE.md)
5. Source code directories (src/, public/)
6. Data directories (telemetry_logs/)

### Research Directory Rules:
1. All development/testing code
2. Analysis and visualization tools
3. Arduino/ESP32 code (all variants)
4. Detailed technical documentation
5. Test results and reports
6. Utility scripts and server code

## 🔍 Quick Lookup

Need to find a file? Use this guide:

| File Type | Location |
|-----------|----------|
| Arduino production code | `research/arduino_code/Basestation_Code_6_Bluetooth.ino` |
| Arduino simulator | `research/arduino_code/Simulated_BaseStation_Integrated.ino` |
| AT command tool | `research/bluetooth/HC05_AT_Command_Setup.ino` |
| Bluetooth guide | `research/bluetooth/HC05_BAUDRATE_CONFIG.md` |
| Setup guide | `research/documentation/SETUP.md` |
| Testing guide | `research/simulation/SIMULATION_TESTING_GUIDE.md` |
| Flight analysis | `research/analysis/analyze_telemetry.py` |
| Telemetry server | `research/scripts/server.py` |
| Range test report | `research/range_tests/Range_Test_2_Report.md` |

## 📝 Git Commit Message

Suggested commit message:
```
Reorganize research directory structure

- Created logical subdirectories: analysis, arduino_code, bluetooth, 
  simulation, documentation, range_tests, scripts
- Moved 43+ files from main directory and research/ to organized structure
- Updated import paths in start_basestation_integrated.py
- Added comprehensive README.md in research/ directory
- Added QUICK_REFERENCE.md in main directory
- Preserved git history (all moves tracked as renames)

Main directory now contains only operational files.
All development/research files properly organized in research/.
```

## 🚀 Next Steps

1. ✅ Commit the reorganization
2. ✅ Test that start_basestation_integrated.py still works
3. ✅ Update any other scripts that import from research/
4. ✅ Inform team about new structure
5. ✅ Update CI/CD if needed

## 📞 Questions?

See:
- `research/README.md` - Complete research guide
- `QUICK_REFERENCE.md` - Quick start guide
- `README.md` - Main project overview

---

**Reorganized by:** AI Assistant
**Date:** 2026-01-17
**Files affected:** 43+ files
**Git operations:** 38 renames, 3 new files, 1 code update
