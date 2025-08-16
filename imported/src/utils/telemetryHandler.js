// telemetryHandler.js
class TelemetryHandler {
    constructor(options = {}) {
      this.apiEndpoint = options.apiEndpoint || 'http://localhost:3000/api/telemetry';
      this.retentionDays = options.retentionDays || 7;
      this.batchSize = options.batchSize || 100;
      this.telemetryQueue = [];
      this.isSending = false;
      this.startTelemetryBatching();
    }
  
    startTelemetryBatching() {
      setInterval(async () => {
        if (this.telemetryQueue.length > 0 && !this.isSending) {
          await this.sendTelemetry();
        }
      }, 5000);
    }
  
    async handleNewTelemetry(telemetryEntry) {
      this.telemetryQueue.push(telemetryEntry);
      
      if (this.telemetryQueue.length >= this.batchSize) {
        await this.sendTelemetry();
      }
    }
  
    async sendTelemetry() {
      if (this.telemetryQueue.length === 0 || this.isSending) return;
  
      this.isSending = true;
      const telemetryToSend = this.telemetryQueue.splice(0, this.batchSize);
  
      try {
        const response = await fetch(this.apiEndpoint, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            telemetry: telemetryToSend,
            retentionDays: this.retentionDays
          }),
        });
  
        if (!response.ok) {
          this.telemetryQueue.unshift(...telemetryToSend);
          throw new Error(`Failed to send telemetry: ${response.statusText}`);
        }
      } catch (error) {
        console.error('Error sending telemetry:', error);
        this.telemetryQueue.unshift(...telemetryToSend);
      } finally {
        this.isSending = false;
      }
    }
  
    async fetchTelemetry(options = {}) {
      const { page = 1, limit = 100, startDate, endDate, minAltitude, maxAltitude } = options;
      
      const queryParams = new URLSearchParams({
        page,
        limit,
        ...(startDate && { startDate: startDate.toISOString() }),
        ...(endDate && { endDate: endDate.toISOString() }),
        ...(minAltitude && { minAltitude }),
        ...(maxAltitude && { maxAltitude })
      });
  
      try {
        const response = await fetch(`${this.apiEndpoint}?${queryParams}`);
        if (!response.ok) throw new Error('Failed to fetch telemetry');
        return await response.json();
      } catch (error) {
        console.error('Error fetching telemetry:', error);
        return { telemetry: [], total: 0 };
      }
    }
  }
  
  export default TelemetryHandler;