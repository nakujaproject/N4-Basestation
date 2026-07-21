// const express = require('express');
// const cors = require('cors');
// const logRoutes = require('./src/routes/logService');

import express from 'express';
import cors from 'cors';
import fs from 'fs';
import path from 'path';
import url from 'url';
import initSqlJs from 'sql.js';
import logRouter from './src/routes/logService.cjs';
import telemetryRouter from './src/routes/telemetryService.cjs';

// Create Express app
const app = express();

// === MBTiles (Node-only, no Python) ===
const __dirname = path.dirname(url.fileURLToPath(import.meta.url));
const MBTILES_FILE = process.env.N4_MBTILES || path.join(__dirname, 'osm-2020-02-10-v3.11_africa_kenya.mbtiles');
let sqlDb = null;
let sqlite = null;

async function loadMbtiles() {
  try {
    if (!fs.existsSync(MBTILES_FILE)) {
      console.warn(`MBTiles not found at ${MBTILES_FILE}. Tile endpoints will 404.`);
      return;
    }
    if (!sqlite) {
      // Ensure Node can find the wasm file
      sqlite = await initSqlJs({
        locateFile: (file) => path.join(__dirname, 'node_modules', 'sql.js', 'dist', file)
      });
    }
    const data = fs.readFileSync(MBTILES_FILE);
    sqlDb = new sqlite.Database(data);
    console.log(`Loaded MBTiles: ${MBTILES_FILE}`);
  } catch (e) {
    console.error('Failed to load MBTiles:', e);
  }
}

function xyzToTmsY(z, y) {
  return (1 << z) - 1 - y;
}

function detectContentType(buf) {
  if (!buf || buf.length < 2) return 'application/octet-stream';
  if (buf.length >= 8 && buf[0] === 0x89 && buf[1] === 0x50) return 'image/png';
  if (buf[0] === 0xff && buf[1] === 0xd8) return 'image/jpeg';
  if (buf[0] === 0x1f && buf[1] === 0x8b) return 'application/x-protobuf'; // gzipped mvt
  return 'application/octet-stream';
}

// Tiles endpoints
app.get(['/metadata.json', '/tiles/metadata.json'], (req, res) => {
  try {
    if (!sqlDb) return res.status(404).json({});
    const stmt = sqlDb.prepare("SELECT value FROM metadata WHERE name='json'");
    const row = stmt.step() ? stmt.getAsObject() : null;
    stmt.free?.();
    if (row && row.value) {
      res.setHeader('Content-Type', 'application/json');
      return res.status(200).send(row.value);
    }
    return res.status(200).json({ vector_layers: [] });
  } catch (e) {
    return res.status(404).json({});
  }
});

app.get(["/:z/:x/:y.:ext", "/tiles/:z/:x/:y.:ext"], (req, res) => {
  try {
    if (!sqlDb) return res.sendStatus(404);
    const z = parseInt(req.params.z, 10);
    const x = parseInt(req.params.x, 10);
    const y = parseInt(req.params.y, 10);
    if (!Number.isFinite(z) || !Number.isFinite(x) || !Number.isFinite(y)) return res.sendStatus(404);
    const y_tms = xyzToTmsY(z, y);
    const stmt = sqlDb.prepare('SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=?');
    stmt.bind([z, x, y_tms]);
    const ok = stmt.step();
    if (!ok) {
      stmt.free?.();
      return res.sendStatus(404);
    }
    const row = stmt.get();
    stmt.free?.();
    const tileData = row?.[0];
    if (!tileData) return res.sendStatus(404);
    const buf = Buffer.from(tileData);
    const ctype = detectContentType(buf);
    res.setHeader('Content-Type', ctype);
    if (ctype === 'application/x-protobuf') {
      res.setHeader('Content-Encoding', 'gzip');
    }
    return res.status(200).send(buf);
  } catch (e) {
    return res.sendStatus(404);
  }
});

// Middleware
app.use(cors());
app.use(express.json());

// Set up routes
app.use('/api/logs', logRouter);

app.use('/api/telemetry', telemetryRouter);

// Error handling middleware
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({
    error: 'Internal Server Error',
    message: process.env.NODE_ENV === 'development' ? err.message : undefined
  });
});

// Handle 404 routes
app.use((req, res) => {
  res.status(404).json({ error: 'Route not found' });
});

// Start server
const PORT = process.env.PORT || 3000;
app.listen(PORT, async () => {
  await loadMbtiles();
  console.log(`Server is running on port ${PORT}`);
});

export default app;