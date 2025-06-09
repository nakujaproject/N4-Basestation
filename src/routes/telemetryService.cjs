// telemetryService.js (Express backend)
const express = require('express');
const fs = require('fs');
const path = require('path');
const { createObjectCsvWriter } = require('csv-writer');

const router = express.Router();

class TelemetryService {
  constructor() {
    this.baseTelemetryPath = path.join(__dirname, '../telemetry');
    this.ensureTelemetryDirectory();
  }

  ensureTelemetryDirectory() {
    if (!fs.existsSync(this.baseTelemetryPath)) {
      fs.mkdirSync(this.baseTelemetryPath, { recursive: true });
    }
  }

  getCurrentTelemetryFile() {
    const date = new Date();
    const fileName = `rocket-telemetry-${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')}.csv`;
    return path.join(this.baseTelemetryPath, fileName);
  }

  async saveTelemetry(telemetryData) {
    const telemetryFile = this.getCurrentTelemetryFile();
    const csvWriter = createObjectCsvWriter({
      path: telemetryFile,
      append: true,
      header: [
        { id: 'timestamp', title: 'TIMESTAMP' },
        { id: 'state', title: 'STATE' },
        { id: 'operationMode', title: 'OPERATION_MODE' },
        { id: 'latitude', title: 'LATITUDE' },
        { id: 'longitude', title: 'LONGITUDE' },
        { id: 'altitude', title: 'ALTITUDE' },
        { id: 'pressure', title: 'PRESSURE' },
        { id: 'temperature', title: 'TEMPERATURE' },
        { id: 'batteryVoltage', title: 'BATTERY_VOLTAGE' },
        { id: 'pyroDrogue', title: 'PYRO_DROGUE' },
        { id: 'pyroMain', title: 'PYRO_MAIN' }
      ]
    });

    await csvWriter.writeRecords(telemetryData.map(entry => ({
      ...entry,
      timestamp: new Date().toISOString()
    })));
  }

  async rotateTelemetry(retentionDays) {
    const files = await fs.promises.readdir(this.baseTelemetryPath);
    const now = new Date();

    for (const file of files) {
      const filePath = path.join(this.baseTelemetryPath, file);
      const stats = await fs.promises.stat(filePath);
      const fileAge = (now - stats.mtime) / (1000 * 60 * 60 * 24);

      if (fileAge > retentionDays) {
        try {
          await fs.promises.unlink(filePath);
          console.log(`Deleted old telemetry file: ${file}`);
        } catch (error) {
          console.error(`Error deleting old telemetry file ${file}:`, error);
        }
      }
    }
  }

  async getTelemetry(options) {
    const { page = 1, limit = 100, startDate, endDate, minAltitude, maxAltitude } = options;
    const telemetry = [];
    const files = await fs.promises.readdir(this.baseTelemetryPath);
    
    for (const file of files) {
      if (!file.endsWith('.csv')) continue;
      
      const filePath = path.join(this.baseTelemetryPath, file);
      const fileContent = await fs.promises.readFile(filePath, 'utf-8');
      const lines = fileContent.split('\n').slice(1);

      for (const line of lines) {
        if (!line.trim()) continue;
        const [timestamp, state, operationMode, latitude, longitude, altitude, 
               pressure, temperature, batteryVoltage, pyroDrogue, pyroMain] = line.split(',');
        
        const entry = {
          timestamp,
          state: parseInt(state),
          operationMode: parseInt(operationMode),
          latitude: parseFloat(latitude),
          longitude: parseFloat(longitude),
          altitude: parseFloat(altitude),
          pressure: parseFloat(pressure),
          temperature: parseFloat(temperature),
          batteryVoltage: parseFloat(batteryVoltage),
          pyroDrogue: parseInt(pyroDrogue),
          pyroMain: parseInt(pyroMain)
        };

        if (startDate && new Date(timestamp) < new Date(startDate)) continue;
        if (endDate && new Date(timestamp) > new Date(endDate)) continue;
        if (minAltitude && entry.altitude < minAltitude) continue;
        if (maxAltitude && entry.altitude > maxAltitude) continue;

        telemetry.push(entry);
      }
    }

    telemetry.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

    const startIndex = (page - 1) * limit;
    const endIndex = startIndex + limit;

    return {
      telemetry: telemetry.slice(startIndex, endIndex),
      total: telemetry.length,
      page,
      totalPages: Math.ceil(telemetry.length / limit)
    };
  }
}

const telemetryService = new TelemetryService();

// API Routes
router.post('/', async (req, res) => {
  try {
    const { telemetry, retentionDays } = req.body;
    await telemetryService.saveTelemetry(telemetry);
    await telemetryService.rotateTelemetry(retentionDays);
    res.status(200).json({ message: 'Telemetry saved successfully' });
  } catch (error) {
    console.error('Error saving telemetry:', error);
    res.status(500).json({ error: 'Failed to save telemetry' });
  }
});

router.get('/', async (req, res) => {
  try {
    const { page, limit, startDate, endDate, minAltitude, maxAltitude } = req.query;
    const telemetry = await telemetryService.getTelemetry({ 
      page, 
      limit,
      startDate,
      endDate,
      minAltitude,
      maxAltitude
    });
    res.status(200).json(telemetry);
  } catch (error) {
    console.error('Error fetching telemetry:', error);
    res.status(500).json({ error: 'Failed to fetch telemetry' });
  }
});

module.exports = router;