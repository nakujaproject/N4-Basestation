import React, { useEffect, useState, useRef } from "react";
import Sidebar from "./components/Sidebar";
import Header from "./components/Header";
import Footer from "./components/Footer";
import Map from "./components/Map";
import Chart from "./components/Chart";
import Video from "./components/Video";
import MQTT from "paho-mqtt";



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
    batteryVoltage: 12,
  });

  // connection status tracking
  const [connectionStatus, setConnectionStatus] = useState({
    baseStation: {
      status: "Disconnected",
    },
    flightComputer: {
      status: "Disconnected",
      lastMessageTime: null,
      messageCount: 0,
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

  
  
  // Handle arming/disarming
  const handleArmRocket = () => {
    if (!mqttClient) {
      console.error("Not connected to MQTT");
      return;
    }

    // Toggle arming state
    const newArmedState = !telemetry.operationMode;
    const message = new MQTT.Message(
      JSON.stringify({
        command: newArmedState ? "ARM" : "DISARM",
      })
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

  // manual connection handler
  const handleConnect = (host, port) => {
    const client = new MQTT.Client(
      host,
      Number(port),
      `dashboard-${Math.random().toString(16).slice(2, 8)}`
    );

    const onConnect = () => {
      console.log("Connected to MQTT Broker");
      client.subscribe(["n4/telemetry", "n4/logs"]);

      // Update connection status
      setConnectionStatus((prev) => ({
        ...prev,
        baseStation: {
          status: "Connected",
          lastConnectionAttempt: new Date(),
          connectionAttempts: prev.baseStation.connectionAttempts + 1,
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
            connectionAttempts: prev.baseStation.connectionAttempts + 1,
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
      client.subscribe(["n4/telemetry", "n4/logs"]);

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
            connectionAttempts: prev.baseStation.connectionAttempts + 1,
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
        const timeSinceLastMessage = (currentTime -  new Date(connectionStatus.flightComputer.lastMessageTime)) / 1000;

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

// using http server 
useEffect(() => {
  const fetchTelemetryData = async () => {
    try {
      const response = await fetch("http://localhost:5000/data");
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      const jsonData = await response.json();

      // Extract relevant telemetry data
      const newTelemetry = {
        state: jsonData.state,
        operationMode: jsonData.operation_mode,
        latitude: jsonData.gps_data.latitude,
        longitude: jsonData.gps_data.longitude,
        altitude: jsonData.gps_data.gps_altitude,
        pressure: jsonData.alt_data.pressure,
        temperature: jsonData.alt_data.temperature,
        pyroDrogue: jsonData.chute_state.pyro1_state,
        pyroMain: jsonData.chute_state.pyro2_state,
        batteryVoltage: jsonData.battery_voltage,
      };

      // Update state
      setTelemetry((prev) => ({
        ...prev,
        ...newTelemetry,
      }));

      // Update charts
      updateCharts(Date.now(), jsonData);

       // Update flight computer connection status
       setConnectionStatus((prev) => ({
        ...prev,
        flightComputer: {
          status: "Connected",
        },
      }));

      setConnectionStatus((prev) => ({
        ...prev,
        baseStation: {
          status: "Connected",
        },
      }));

    } catch (error) {
      console.error("Error fetching telemetry data:", error);
      const errorLog = {
        timestamp: new Date(),
        level: "ERROR",
        message: "Error parsing message: " + error.message,
        source: "Dashboard",
      };

      setArmingLogs((prevLogs) => [errorLog, ...prevLogs]);

      setError("Failed to fetch telemetry data");

      setConnectionStatus((prev) => ({
        ...prev,
        flightComputer: {
          ...prev.flightComputer,
          status: "Data Error",
        },
      }));
    }
  };

  // Fetch telemetry data every 1/100th of asecond
  const interval = setInterval(fetchTelemetryData, 100);

  return () => clearInterval(interval);
}, []);


  // Message arrived handler
  let onMessageArrived =  (message) => {
    try {
      const receivedData = JSON.parse(message.payloadString);

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

      // Update flight computer connection status
      setConnectionStatus((prev) => ({
        ...prev,
        flightComputer: {
          status: "Connected",
        },
      }));

      // Update telemetry state
      setTelemetry((prev) => ({
        ...prev,
        state: receivedData.state,
        operationMode: receivedData.operation_mode,
        latitude: receivedData.gps_data.latitude,
        longitude: receivedData.gps_data.longitude,
        altitude: receivedData.gps_data.gps_altitude,
        pressure: receivedData.alt_data.pressure,
        temperature: receivedData.alt_data.temperature,
        pyroDrogue: receivedData.chute_state.pyro1_state,
        pyroMain: receivedData.chute_state.pyro2_state,
        batteryVoltage: receivedData.battery_voltage,
      }));

      
      try{
        // Update charts
        const time = Date.now();
        updateCharts(time, receivedData);
        
      } catch (err){
        console.error("Error updating charts", err);
      }

    } catch (error) {
      const errorLog = {
        timestamp: new Date(),
        level: "ERROR",
        message: "Error parsing message: " + error.message,
        source: "Dashboard",
      };
      

      setArmingLogs((prevLogs) => [errorLog, ...prevLogs].slice(0, 10)); // Limit to 10 logs


      setError("Error parsing message");
      setConnectionStatus((prev) => ({
        ...prev,
        flightComputer: {
          ...prev.flightComputer,
          status: "Data Error",
        },
      }));
    }
  };

  const updateCharts = (time, received_data) => {
    // Update altitude chart
    altitudeChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.gps_data.gps_altitude, });
    altitudeChartRef.current.data.datasets[1].data.push({ x: time, y: received_data.alt_data.AGL, });
    altitudeChartRef.current.update("quiet");

    // Update velocity chart
    velocityChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.alt_data.velocity,});
    velocityChartRef.current.update("quiet");

    // Update acceleration chart
    accelerationChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.acc_data.ax, }); 
    accelerationChartRef.current.data.datasets[1].data.push({ x: time, y: received_data.acc_data.ay, });
    accelerationChartRef.current.data.datasets[2].data.push({ x: time, y: received_data.acc_data.az, });
    accelerationChartRef.current.update("quiet");
  };

  return (
    <div className="h-full box-border m-0 text-black w-full mx-auto ">
      <main className="flex flex-col md:flex-row md:space-y-0 w-full h-screen selection:bg-blue-600 ">
        <div className="md:w-1/4 h-screen">
          <Sidebar
            state={telemetry.state}
            operationMode={telemetry.operationMode}
            altitude={telemetry.altitude}
            pressure={telemetry.pressure}
            temperature={telemetry.temperature}
            pyroDrogue={telemetry.pyroDrogue}
            pyroMain={telemetry.pyroMain}
            onConnect={handleConnect}
            connectionStatus={connectionStatus}
            onArmRocket={handleArmRocket}
            armingLogs={armingLogs}
          />
        </div>
        <div className="w-full md:w-3/4">
          <Header
            connectionStatus={connectionStatus}
            batteryVoltage={telemetry.batteryVoltage}
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
