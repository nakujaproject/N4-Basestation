// logHandler.js
class LogHandler {
    constructor(options = {}) {
      this.maxUiLogs = options.maxUiLogs || 10;
      this.apiEndpoint = options.apiEndpoint || 'http://localhost:3000/api/logs';
      this.retentionDays = options.retentionDays || 7;
      this.batchSize = options.batchSize || 50;
      this.logQueue = [];
      this.isSending = false;
  
      // Start periodic log sending
      this.startLogBatching();
    }
  
    async handleNewLog(logEntry) {
      // Add to queue for batched sending
      this.logQueue.push({
        ...logEntry,
        timestamp: new Date().toISOString()
      });
  
      // If queue is getting large, trigger immediate send
      if (this.logQueue.length >= this.batchSize) {
        await this.sendLogs();
      }
  
      // Return logs for UI update
      return logEntry;
    }
  
    // Manage logs shown in UI
    manageUiLogs(currentLogs, newLog) {
      const updatedLogs = [newLog, ...currentLogs];
      return updatedLogs.slice(0, this.maxUiLogs);
    }
  
    // Start periodic log sending
    startLogBatching() {
      setInterval(async () => {
        if (this.logQueue.length > 0 && !this.isSending) {
          await this.sendLogs();
        }
      }, 5000); // Send logs every 5 seconds if there are any
    }
  
    // Send logs to backend
    async sendLogs() {
      if (this.logQueue.length === 0 || this.isSending) return;
  
      this.isSending = true;
      const logsToSend = this.logQueue.splice(0, this.batchSize);
  
      try {
        const response = await fetch('http://localhost:3000/api/logs', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            logs: logsToSend,
            retentionDays: this.retentionDays
          }),
        });
  
        if (!response.ok) {
          // If send fails, put logs back in queue
          this.logQueue.unshift(...logsToSend);
          throw new Error(`Failed to send logs: ${response.statusText}`);
        }
      } catch (error) {
        console.error('Error sending logs:', error);
        // Put logs back in queue on error
        this.logQueue.unshift(...logsToSend);
      } finally {
        this.isSending = false;
      }
    }
  
    // Get logs for display
    async fetchLogs(options = {}) {
      const { page = 1, limit = 50, level, startDate, endDate } = options;
      
      const queryParams = new URLSearchParams({
        page,
        limit,
        ...(level && { level }),
        ...(startDate && { startDate: startDate.toISOString() }),
        ...(endDate && { endDate: endDate.toISOString() })
      });
  
      try {
        const response = await fetch(`${this.apiEndpoint}?${queryParams}`);
        if (!response.ok) throw new Error('Failed to fetch logs');
        return await response.json();
      } catch (error) {
        console.error('Error fetching logs:', error);
        return { logs: [], total: 0 };
      }
    }
  }
  
  export default LogHandler;