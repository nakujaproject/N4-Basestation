import React, { useEffect, useState, useRef } from "react";
import Sidebar from "./components/Sidebar";
import Header from "./components/Header";
import Footer from "./components/Footer";
import Map from "./components/Map";
import Chart from "./components/Chart";
import Video from "./components/Video";
import MQTT from "paho-mqtt";

// Removed throttling variables for immediate updates
const MAX_POINTS = 2000; // max points to keep in each chart

function App() {
  const [telemetry, setTelemetry] = useState({
    state: 0,
    operationMode: 0,
    latitude: -1.1,
    longitude: 37.01,
    pressure: 0,
    temperature: 0,
    pyroDrogue: 0,
    pyroMain: 0,
    altitude: 0,
    altitudeAGL: 0, // Added AGL altitude from barometric sensor
    batteryVoltage: 0, // Changed default from 12 to 0 to match actual data
    rssi: 0,
    velocity: 0,
    recordNumber: 0,
    // Add new telemetry fields
    acceleration: { ax: 0, ay: 0, az: 0, pitch: 0, roll: 0 },
    gyro: { gx: 0, gy: 0, gz: 0 },
    kalman: { altitude: 0, verticalVelocity: 0 }, // Added Kalman filter data
    communicationMode: "MQTT", // Track if using MQTT or Beacon mode - default to MQTT
    packetsReceived: 0, // Track total packets from base station
  });

  // Enhanced connection status tracking
  const [connectionStatus, setConnectionStatus] = useState({
    baseStation: {
      status: "Disconnected",
    },
    flightComputer: {
      status: "Disconnected",
      lastMessageTime: null,
      messageCount: 0,
      rssi: 0,
      communicationMode: "MQTT", // MQTT or Beacon - default to MQTT
      signalQuality: "Unknown", // Good, Fair, Poor based on RSSI
    },
  });

  const [error, setError] = useState(null);
  const [armingLogs, setArmingLogs] = useState([]);

  const altitudeChartRef = useRef(null);
  const velocityChartRef = useRef(null);
  const accelerationChartRef = useRef(null);

  const mqtt_host = import.meta.env.VITE_MQTT_HOST;
  const ws_port = Number(import.meta.env.VITE_WS_PORT);
  const [mqttClient, setMqttClient] = useState(null);

  // Helper function to determine signal quality from RSSI
  const getSignalQuality = (rssi) => {
    if (rssi === 0) return "No Signal";
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Poor";
    return "Very Poor";
  };

  // Handle arming/disarming
  const handleArmRocket = () => {
    if (!mqttClient) {
      console.error("Not connected to MQTT");
      return;
    }

    // Toggle arming state
    const newArmedState = !telemetry.operationMode;
    const message = new MQTT.Message(
      newArmedState ? "ARM" : "DISARM",
    );
    message.destinationName = "n4/commands";

    try {
      mqttClient.send(message);

      // Log the arming action
      const logEntry = {
        timestamp: new Date(),
        action: newArmedState ? "Armed" : "Disarmed",
        status: "Sent",
        level: "INFO",
        message: `Rocket ${newArmedState ? "armed" : "disarmed"} successfully`,
        source: "Basestation",
      };

      setArmingLogs((prev) => [logEntry, ...prev]);

      // Optimistically update the telemetry state
      setTelemetry((prev) => ({
        ...prev,
        operationMode: newArmedState,
      }));
    } catch (err) {
      const logEntry = {
        timestamp: new Date(),
        action: newArmedState ? "Arm" : "Disarm",
        status: "Failed",
        level: "ERROR",
        message: `Command failed: ${err.message}`,
        source: "Basestation",
      };

      setArmingLogs((prev) => [logEntry, ...prev]);
      setError("Failed to send arming command");
    }
  };

  // Handle command sending from the sidebar
  const handleSendCommand = (command) => {
    if (!mqttClient) {
      setError("MQTT not connected. Cannot send command.");
      return;
    }

    // Map frontend command to backend-expected string
    let backendCommand = "";
    switch (command.toUpperCase()) {
      case "ARM":
      case "DISARM":
      case "RESET":
        backendCommand = command.toUpperCase();
        break;
      case "MQTT":
        backendCommand = "CMD_MQTT_MODE";
        break;
      case "BEACON":
        backendCommand = "CMD_BEACON_MODE";
        break;
      case "DUAL":
        backendCommand = "CMD_DUAL_MODE";
        break;
      case "AUTO_ON":
        backendCommand = "CMD_AUTO_ON";
        break;
      case "AUTO_OFF":
        backendCommand = "CMD_AUTO_OFF";
        break;
      case "STATUS":
        backendCommand = "CMD_STATUS";
        break;
      default:
        backendCommand = command.toUpperCase();
    }

    const message = new MQTT.Message(backendCommand);
    message.destinationName = "n4/commands";

    try {
      mqttClient.send(message);

      // Log the command action
      const logEntry = {
        timestamp: new Date(),
        action: `Command: ${backendCommand}`,
        status: "Sent",
        level: "INFO",
        message: `Command \"${backendCommand}\" sent successfully`,
        source: "Basestation",
      };

      setArmingLogs((prev) => [logEntry, ...prev]);

    } catch (err) {
      const logEntry = {
        timestamp: new Date(),
        action: `Command: ${backendCommand}`,
        status: "Failed",
        level: "ERROR",
        message: `Command failed: ${err.message}`,
        source: "Basestation",
      };

      setArmingLogs((prev) => [logEntry, ...prev]);
      setError(`Failed to send command: ${backendCommand}`);
    }
  };

  // manual connection handler
  const handleConnect = (host, port) => {
    const client = new MQTT.Client(
      host,
      Number(port),
      `dashboard-${Math.random().toString(16).slice(2, 8)}`
    );

    const onConnect = () => {
      console.log("Connected to MQTT Broker");
      client.subscribe(["n4/app/flight-computer-1", "n4/app/logs"]);

      // Update connection status
      setConnectionStatus((prev) => ({
        ...prev,
        baseStation: {
          status: "Connected",
          lastConnectionAttempt: new Date(),
          connectionAttempts: (prev.baseStation.connectionAttempts || 0) + 1,
        },
      }));
    };

    // Connect the client
    client.connect({
      onSuccess: onConnect,
      keepAliveInterval: 3600,
      onFailure: (error) => {
        console.error("Connection failed", error);
        setConnectionStatus((prev) => ({
          ...prev,
          baseStation: {
            status: "Connection Failed",
            lastConnectionAttempt: new Date(),
            connectionAttempts: (prev.baseStation.connectionAttempts || 0) + 1,
          },
        }));
      },
    });

    // Set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    setMqttClient(client);

    return () => {
      client.disconnect();
    };
  };

  useEffect(() => {
    const client = new MQTT.Client(
      mqtt_host,
      ws_port,
      `dashboard-${new Date().getTime().toString().slice(4)}`
    );

    const onConnect = () => {
      console.log("Connected to MQTT Broker");
      client.subscribe(["n4/app/flight-computer-1", "n4/app/logs"]);

      // Update connection status
      setConnectionStatus((prev) => ({
        ...prev,
        baseStation: {
          status: "Connected",
        },
      }));
    };

    // Connect the client
    client.connect({
      onSuccess: onConnect,
      keepAliveInterval: 3600,
      onFailure: (error) => {
        console.error("Connection failed", error);
        setConnectionStatus((prev) => ({
          ...prev,
          baseStation: {
            status: "Disconnected",
            lastConnectionAttempt: new Date(),
            connectionAttempts: (prev.baseStation.connectionAttempts || 0) + 1,
          },
        }));
      },
    });

    // Set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    setMqttClient(client);

    // Data staleness check
    const dataStaleInterval = setInterval(() => {
      const currentTime = new Date();
      if (connectionStatus.flightComputer.lastMessageTime) {
        const timeSinceLastMessage = (currentTime - new Date(connectionStatus.flightComputer.lastMessageTime)) / 1000;

        if (timeSinceLastMessage > 5) {
          setConnectionStatus((prev) => ({
            ...prev,
            flightComputer: {
              ...prev.flightComputer,
              status: "No Recent Data",
            },
          }));
        }
      }
    }, 500);

    return () => {
      client.disconnect();
      clearInterval(dataStaleInterval);
    };
  }, []);

  // Connection lost handler
  let onConnectionLost = (responseObject) => {
    if (responseObject.errorCode !== 0) {
      setError("Connection lost: " + responseObject.errorMessage);
      setConnectionStatus((prev) => ({
        ...prev,
        baseStation: {
          ...prev.baseStation,
          status: "Disconnected",
        },
      }));
    }
  };

  // Message arrived handler
  let onMessageArrived = (message) => {
    const payload = message.payloadString;

    try {
      const receivedData = JSON.parse(message.payloadString);
      // Debug: log the full received data and communication_mode
      console.log("[DEBUG] Received telemetry:", receivedData);
      console.log("[DEBUG] communication_mode field:", receivedData.communication_mode);

      // Check if the message is a log message
      if (message.destinationName === "n4/logs") {
        const newLog = {
          timestamp: new Date(),
          level: receivedData.level || "INFO",
          message: receivedData.message,
          source: receivedData.source || "Flight Computer",
        };

        // Add log to flight computer logs
        setArmingLogs((prevLogs) => [newLog, ...prevLogs].slice(0, 10)); // Limit to 10 logs
        return;
      }

      // Determine communication mode and signal quality
      let commMode = "MQTT"; // Default to MQTT
      if (receivedData.communication_mode) {
        let modeStr = String(receivedData.communication_mode).trim().toLowerCase();
        if (modeStr === "beacon") {
          commMode = "Beacon";
        } else if (modeStr === "mqtt") {
          commMode = "MQTT";
        } else {
          commMode = "MQTT"; // fallback to MQTT for unknown values
        }
      }
      const signalQuality = getSignalQuality(receivedData.wifi_rssi || 0);

      // Update flight computer connection status with enhanced info
      setConnectionStatus((prev) => ({
        ...prev,
        flightComputer: {
          status: "Connected",
          lastMessageTime: new Date(),
          messageCount: prev.flightComputer.messageCount + 1,
          rssi: receivedData.wifi_rssi || 0,
          communicationMode: commMode,
          signalQuality: signalQuality,
        },
      }));

      // Update telemetry state from JSON with all fields
      setTelemetry((prev) => ({
        ...prev,
        state: receivedData.state || 0,
        operationMode: receivedData.operation_mode || 0,
        latitude: receivedData.gps_data?.latitude || 0,
        longitude: receivedData.gps_data?.longitude || 0,
        altitude: receivedData.gps_data?.gps_altitude || 0, // GPS altitude
        altitudeAGL: receivedData.alt_data?.AGL || 0, // Barometric altitude AGL
        pressure: receivedData.alt_data?.pressure || 0,
        temperature: receivedData.alt_data?.temperature || 0,
        pyroDrogue: receivedData.chute_state?.pyro1_state || 0,
        pyroMain: receivedData.chute_state?.pyro2_state || 0,
        batteryVoltage: receivedData.battery_voltage || 0,
        rssi: receivedData.wifi_rssi || 0,
        velocity: receivedData.alt_data?.velocity || 0,
        recordNumber: receivedData.record_number || 0,
        // Update acceleration and gyro data
        acceleration: {
          ax: receivedData.acc_data?.ax || 0,
          ay: receivedData.acc_data?.ay || 0,
          az: receivedData.acc_data?.az || 0,
          pitch: receivedData.acc_data?.pitch || 0,
          roll: receivedData.acc_data?.roll || 0,
        },
        gyro: {
          gx: receivedData.gyro_data?.gx || 0,
          gy: receivedData.gyro_data?.gy || 0,
          gz: receivedData.gyro_data?.gz || 0,
        },
        kalman: {
          altitude: receivedData.kalman_data?.altitude || 
                   receivedData.kalman_altitude || 
                   receivedData.alt_data?.kalman_altitude || 0,
          verticalVelocity: receivedData.kalman_data?.vertical_velocity || 
                           receivedData.kalman_vertical_velocity || 
                           receivedData.alt_data?.kalman_vertical_velocity || 0,
        },
        communicationMode: commMode, // Always 'Beacon' or 'MQTT'
        packetsReceived: receivedData.packets_received || 0,
      }));

      try {
        // Update charts
        const time = Date.now();
        updateCharts(time, receivedData);
      } catch (err) {
        console.error("Error updating charts", err);
      }

    } catch (jsonError) {
      // If JSON parsing fails, try parsing CSV with your 25-field format
      try {
        const values = payload.trim().split(',');
        
        // Ensure we have the expected number of fields (25 fields total)
        if (values.length < 25) {
          throw new Error(`Expected 25 CSV fields, got ${values.length}`);
        }

        // Convert string values to numbers where appropriate
        const numericValues = values.map((val, index) => {
          // GPS coordinates need higher precision
          if (index === 11 || index === 12) { // latitude, longitude
            return parseFloat(val);
          }
          return parseFloat(val) || 0;
        });

        // Map the CSV to telemetry structure based on your flight computer format:
        const receivedData = {
          record_number: numericValues[0],
          operation_mode: numericValues[1],
          state: numericValues[2],
          acc_data: {
            ax: numericValues[3],
            ay: numericValues[4],
            az: numericValues[5],
            pitch: numericValues[6],
            roll: numericValues[7],
          },
          gyro_data: {
            gx: numericValues[8],
            gy: numericValues[9],
            gz: numericValues[10],
          },
          gps_data: {
            latitude: numericValues[11],
            longitude: numericValues[12],
            gps_altitude: numericValues[13],
            time: numericValues[14],
          },
          alt_data: {
            pressure: numericValues[15],
            temperature: numericValues[16],
            AGL: numericValues[17],
            velocity: numericValues[18],
          },
          chute_state: {
            pyro1_state: numericValues[19],
            pyro2_state: numericValues[20],
          },
          battery_voltage: numericValues[21],
          wifi_rssi: numericValues[22],
          kalman_data: {
            altitude: numericValues[23],
            vertical_velocity: numericValues[24],
          },
          communication_mode: "MQTT", // Default to MQTT for CSV data
        };

        // Determine signal quality
        const signalQuality = getSignalQuality(receivedData.wifi_rssi);

        setConnectionStatus((prev) => ({
          ...prev,
          flightComputer: {
            status: "Connected",
            lastMessageTime: new Date(),
            messageCount: prev.flightComputer.messageCount + 1,
            rssi: receivedData.wifi_rssi,
            communicationMode: receivedData.communication_mode,
            signalQuality: signalQuality,
          },
        }));

        // Update telemetry from CSV-mapped structure
        // Determine commMode for CSV as well (since CSV sets communication_mode: 'MQTT' by default)
        let commMode = "MQTT";
        if (receivedData.communication_mode) {
          let modeStr = String(receivedData.communication_mode).trim().toLowerCase();
          if (modeStr === "beacon") {
            commMode = "Beacon";
          } else if (modeStr === "mqtt") {
            commMode = "MQTT";
          } else {
            commMode = "MQTT"; // fallback to MQTT for unknown values
          }
        }
        setTelemetry((prev) => ({
          ...prev,
          state: receivedData.state,
          operationMode: receivedData.operation_mode,
          latitude: receivedData.gps_data.latitude,
          longitude: receivedData.gps_data.longitude,
          altitude: receivedData.gps_data.gps_altitude,
          altitudeAGL: receivedData.alt_data.AGL,
          pressure: receivedData.alt_data.pressure,
          temperature: receivedData.alt_data.temperature,
          pyroDrogue: receivedData.chute_state.pyro1_state,
          pyroMain: receivedData.chute_state.pyro2_state,
          batteryVoltage: receivedData.battery_voltage,
          rssi: receivedData.wifi_rssi,
          velocity: receivedData.alt_data.velocity,
          recordNumber: receivedData.record_number,
          acceleration: {
            ax: receivedData.acc_data.ax,
            ay: receivedData.acc_data.ay,
            az: receivedData.acc_data.az,
            pitch: receivedData.acc_data.pitch,
            roll: receivedData.acc_data.roll,
          },
          gyro: {
            gx: receivedData.gyro_data.gx,
            gy: receivedData.gyro_data.gy,
            gz: receivedData.gyro_data.gz,
          },
          kalman: {
            altitude: receivedData.kalman_data.altitude,
            verticalVelocity: receivedData.kalman_data.vertical_velocity,
          },
          communicationMode: commMode, // Always 'Beacon' or 'MQTT'
        }));

        // Update charts from parsed CSV
        const time = Date.now();
        updateCharts(time, receivedData);

      } catch (csvError) {
        // Failed to parse as both JSON and CSV
        console.error("CSV parsing error:", csvError);
        setError(`Error parsing message: ${csvError.message}`);
        setConnectionStatus((prev) => ({
          ...prev,
          flightComputer: {
            ...prev.flightComputer,
            status: "Data Error",
          },
        }));
      }
    }
  };

  const updateCharts = (time, received_data) => {
    // Remove throttling for immediate updates
    // Check if chart refs exist before updating
    if (!altitudeChartRef.current || !velocityChartRef.current || !accelerationChartRef.current) {
      return;
    }

    // --- Altitude (GPS vs Barometric vs Kalman) ---
    const alt0 = altitudeChartRef.current.data.datasets[0].data;
    const alt1 = altitudeChartRef.current.data.datasets[1].data;
    const alt2 = altitudeChartRef.current.data.datasets[2].data;
    
    // Extract Kalman altitude from multiple possible locations
    const kalmanAlt = received_data.kalman_data?.altitude || 
                     received_data.kalman_altitude || 
                     received_data.alt_data?.kalman_altitude || 0;
    
    alt0.push({ x: time, y: received_data.gps_data?.gps_altitude || 0 });
    alt1.push({ x: time, y: received_data.alt_data?.AGL || 0 });
    alt2.push({ x: time, y: kalmanAlt });
    
    if (alt0.length > MAX_POINTS) alt0.shift();
    if (alt1.length > MAX_POINTS) alt1.shift();
    if (alt2.length > MAX_POINTS) alt2.shift();
    altitudeChartRef.current.update("quiet");

    // --- Velocity (Barometric vs Kalman) ---
    const vel0 = velocityChartRef.current.data.datasets[0].data;
    const vel1 = velocityChartRef.current.data.datasets[1].data;
    
    // Extract Kalman vertical velocity from multiple possible locations
    const kalmanVel = received_data.kalman_data?.vertical_velocity || 
                     received_data.kalman_vertical_velocity || 
                     received_data.alt_data?.kalman_vertical_velocity || 0;
    
    vel0.push({ x: time, y: received_data.alt_data?.velocity || 0 });
    vel1.push({ x: time, y: kalmanVel });
    
    if (vel0.length > MAX_POINTS) vel0.shift();
    if (vel1.length > MAX_POINTS) vel1.shift();
    velocityChartRef.current.update("quiet");

    // --- Acceleration ---
    const acc0 = accelerationChartRef.current.data.datasets[0].data;
    const acc1 = accelerationChartRef.current.data.datasets[1].data;
    const acc2 = accelerationChartRef.current.data.datasets[2].data;
    acc0.push({ x: time, y: received_data.acc_data?.ax || 0 });
    acc1.push({ x: time, y: received_data.acc_data?.ay || 0 });
    acc2.push({ x: time, y: received_data.acc_data?.az || 0 });
    if (acc0.length > MAX_POINTS) acc0.shift();
    if (acc1.length > MAX_POINTS) acc1.shift();
    if (acc2.length > MAX_POINTS) acc2.shift();
    accelerationChartRef.current.update("quiet");
  };

  return (
    <div className="h-full box-border m-0 text-black w-full mx-auto ">
      <main className="flex flex-col md:flex-row md:space-y-0 w-full h-screen selection:bg-blue-600 ">
        <div className="md:w-1/4 h-screen">
          <Sidebar
            state={telemetry.state}
            operationMode={telemetry.operationMode}
            altitude={telemetry.altitudeAGL} // Use barometric altitude for primary display
            pressure={telemetry.pressure}
            temperature={telemetry.temperature}
            pyroDrogue={telemetry.pyroDrogue}
            pyroMain={telemetry.pyroMain}
            velocity={telemetry.velocity} // Pass velocity to sidebar if needed
            recordNumber={telemetry.recordNumber} // Pass record number for packet tracking
            rssi={telemetry.rssi} // Pass RSSI to sidebar
            communicationMode={telemetry.communicationMode} // Pass communication mode to sidebar
            onConnect={handleConnect}
            connectionStatus={connectionStatus}
            onArmRocket={handleArmRocket}
            onSendCommand={handleSendCommand} // Add command handler
            isConnected={connectionStatus.baseStation.status === "Connected"} // Add connection status
            armingLogs={armingLogs}
          />
        </div>
        <div className="w-full md:w-3/4">
          <Header
            connectionStatus={connectionStatus}
            batteryVoltage={telemetry.batteryVoltage}
            rssi={telemetry.rssi}
            communicationMode={telemetry.communicationMode} // Pass communication mode
            recordNumber={telemetry.recordNumber} // Pass record number
            packetsReceived={telemetry.packetsReceived} // Pass total packets
          />
          <div className="mt-10 md:mt-16 grid grid-cols-1 md:grid-cols-2 gap-2 w-full p-2">
            <div className="flex flex-col space-y-2">
              <Chart ref={altitudeChartRef} type="altitude" />
              <Chart ref={velocityChartRef} type="velocity" />
            </div>
            <div className="flex flex-col space-y-2">
              <div className="h-full w-full">
                <Video />
              </div>
              <Chart ref={accelerationChartRef} type="acceleration" />
            </div>
          </div>
          <div className="flex w-full p-2">
            <div className="h-[500px] w-[1555px] z-0 ">
              <Map position={[telemetry.latitude, telemetry.longitude]} />
            </div>
          </div>
          <Footer />
        </div>
      </main>
    </div>
  );
}

export default App;