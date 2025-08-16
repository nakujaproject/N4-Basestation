// logService.js (Express backend)
const express = require('express');
const fs = require('fs');
const path = require('path');
const { createObjectCsvWriter } = require('csv-writer');

// import express from 'express';
// import fs from 'fs';
// import path from 'path';
// import { createObjectCsvWriter } from 'csv-writer';


const router = express.Router();

class LogService {
  constructor() {
    this.baseLogPath = path.join(__dirname, '../logs');
    this.ensureLogDirectory();
  }

  ensureLogDirectory() {
    if (!fs.existsSync(this.baseLogPath)) {
      fs.mkdirSync(this.baseLogPath, { recursive: true });
    }
  }

  getCurrentLogFile() {
    const date = new Date();
    const fileName = `rocket-telemetry-${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')}.csv`;
    return path.join(this.baseLogPath, fileName);
  }

  async saveLogs(logs) {
    const logFile = this.getCurrentLogFile();
    const csvWriter = createObjectCsvWriter({
      path: logFile,
      append: true,
      header: [
        { id: 'timestamp', title: 'TIMESTAMP' },
        { id: 'level', title: 'LEVEL' },
        { id: 'source', title: 'SOURCE' },
        { id: 'message', title: 'MESSAGE' },
        { id: 'action', title: 'ACTION' },
        { id: 'status', title: 'STATUS' }
      ]
    });

    await csvWriter.writeRecords(logs);
  }

  async rotateLogs(retentionDays) {
    const files = await fs.promises.readdir(this.baseLogPath);
    const now = new Date();

    for (const file of files) {
      const filePath = path.join(this.baseLogPath, file);
      const stats = await fs.promises.stat(filePath);
      const fileAge = (now - stats.mtime) / (1000 * 60 * 60 * 24); // age in days

      if (fileAge > retentionDays) {
        try {
          await fs.promises.unlink(filePath);
          console.log(`Deleted old log file: ${file}`);
        } catch (error) {
          console.error(`Error deleting old log file ${file}:`, error);
        }
      }
    }
  }

  async getLogs(options) {
    const { page = 1, limit = 50, level, startDate, endDate } = options;
    const logs = [];
    const files = await fs.promises.readdir(this.baseLogPath);
    
    for (const file of files) {
      if (!file.endsWith('.csv')) continue;
      
      const filePath = path.join(this.baseLogPath, file);
      const fileContent = await fs.promises.readFile(filePath, 'utf-8');
      const lines = fileContent.split('\n').slice(1); // Skip header

      for (const line of lines) {
        if (!line.trim()) continue;
        const [timestamp, logLevel, source, message, action, status] = line.split(',');
        
        if (level && logLevel !== level) continue;
        if (startDate && new Date(timestamp) < new Date(startDate)) continue;
        if (endDate && new Date(timestamp) > new Date(endDate)) continue;

        logs.push({ timestamp, level: logLevel, source, message, action, status });
      }
    }

    // Sort logs by timestamp descending
    logs.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

    const startIndex = (page - 1) * limit;
    const endIndex = startIndex + limit;

    return {
      logs: logs.slice(startIndex, endIndex),
      total: logs.length,
      page,
      totalPages: Math.ceil(logs.length / limit)
    };
  }
}

const logService = new LogService();

// API Routes
router.post('/', async (req, res) => {
  try {
    const { logs, retentionDays } = req.body;
    await logService.saveLogs(logs);
    await logService.rotateLogs(retentionDays);
    res.status(200).json({ message: 'Logs saved successfully' });
  } catch (error) {
    console.error('Error saving logs:', error);
    res.status(500).json({ error: 'Failed to save logs' });
  }
});

router.get('/', async (req, res) => {
  try {
    const { page, limit, level, startDate, endDate } = req.query;
    const logs = await logService.getLogs({ page, limit, level, startDate, endDate });
    res.status(200).json(logs);
  } catch (error) {
    console.error('Error fetching logs:', error);
    res.status(500).json({ error: 'Failed to fetch logs' });
  }
});

module.exports = router;