// const express = require('express');
// const cors = require('cors');
// const logRoutes = require('./src/routes/logService');

import express from 'express';
import cors from 'cors';
import  logRouter  from './src/routes/logService.cjs';
import telemetryRouter from './src/routes/telemetryService.cjs';

// Create Express app
const app = express();

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
app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});

export default app;