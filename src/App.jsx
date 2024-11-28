import React, { useEffect, useState, useRef } from 'react';
import Sidebar from './components/Sidebar';
import Header from './components/Header';
import Footer from './components/Footer';
import Map from './components/Map';
import Chart from './components/Chart';
import Video from './components/Video';
import MQTT from 'paho-mqtt';



function App() {

  let [isConnected, setIsConnected] = useState(false);
  let [batteryVoltage, setBatteryVoltage] = useState(12);
  let [state, setState] = useState(0);
  let [operationMode, setOperationMode] = useState(0);
  let [latitude, setLatitude] = useState(-1.1);
  let [longitude, setLongitude] = useState(37.01);
  let [pressure, setPressure] = useState(0);
  let [temperature, setTemperature] = useState(0);
  let [pyroDrogue, setPyroDrougue] = useState(0);
  let [pyroMain, setPyroMain] = useState(0);
  let [altitude, setAltitude] = useState(0);
  let [error, setError] = useState(null);

  const altitudeChartRef = useRef(null);
	const velocityChartRef = useRef(null);
	const accelerationChartRef = useRef(null);
  
  let mqtt_host = import.meta.env.VITE_MQTT_HOST;
  let ws_port = Number(import.meta.env.VITE_WS_PORT);
  // console.log(mqtt_host);

  let handleConnect = (host, port) => {
    
    const new_client = new MQTT.Client(
    host,
    Number(port),
    `dashboard-${Math.random().toString(16).slice(2, 8)}`
    )

    let onConnect = () => {
      console.log("Connected");
      new_client.subscribe("n4/telemetry");
      setIsConnected(true);
    }

    // Connect the client
    new_client.connect({
      onSuccess: onConnect,
      keepAliveInterval: 3600,
    });

    // Set callback handlers
    new_client.onConnectionLost = onConnectionLost;
    new_client.onMessageArrived = onMessageArrived;

    return () => {
      client.disconnect();
    }


   
  }

  useEffect(()=>{
    const client = new MQTT.Client(
      mqtt_host,
      ws_port,
      `dashboard-${((new Date()).getTime()).toString().slice(4)}`
    )

    let onConnect = () => {
      console.log("Connected");
      client.subscribe("n4/telemetry");
      setIsConnected(true);
    }

    // Connect the client
    client.connect({
      onSuccess: onConnect,
      keepAliveInterval: 3600,
    });

    // Set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    return () => {
      client.disconnect();
    }
  }, []);

  // Called when the client loses its connection
	let onConnectionLost = (responseObject) => {
		if (responseObject.errorCode !== 0) {
			setError("Connection lost:" + responseObject.errorMessage);
		}	
    setIsConnected(false);
	};

  // Called when a message arrives
	let onMessageArrived = (message) => {
			try {
        // parse data in json format to telemetry_data packet
        // Example output -> { "id": 123, "state": 1, "operation_mode": 2, 
        // "acc_data": { "ax": 1.23, "ay": 4.56, "az": 7.89, "pitch": 10.11, "roll": 12.13 }, 
        // "gyro_data": { "gx": 14.15, "gy": 16.17, "gz": 18.19 }, 
        // "gps_data": { "latitude": 20.2, "longitude": 22.2, "gps_altitude": 24.23, "time": 25 }, 
        // "alt_data": { "pressure": 26.24, "temperature": 28.25, "AGL": 30.26, "velocity": 32.27 }, 
        // "chute_state": { "pyro1_state": 1, "pyro2_state": 0 }, "battery_voltage": 34.28 }

				let receivedData = JSON.parse(message.payloadString)
				let time = Date.now();
	
				// console.log(receivedData);
        setState(receivedData.state);
        setOperationMode(receivedData.operation_mode);
        setLatitude(receivedData.gps_data.latitude);
        setLongitude(receivedData.gps_data.longitude);
        setAltitude(receivedData.gps_data.gps_altitude);
        setPressure(receivedData.alt_data.pressure);
        setTemperature(receivedData.alt_data.temperature);
        setPyroDrougue(receivedData.chute_state.pyro1_state);
        setPyroMain(receivedData.chute_state.pyro2_state);
        setBatteryVoltage(receivedData.battery_voltage); 

				updateCharts(time, receivedData);
	
			} catch (error) {
				setError('Error parsing message');
        // console.log(error);
			}
		
			
	}

	const updateCharts = (time, received_data) => {
		// Update altitude chart
		altitudeChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.gps_data.gps_altitude });
		altitudeChartRef.current.data.datasets[1].data.push({ x: time, y: received_data.alt_data.AGL });
		altitudeChartRef.current.update('quiet');

		// Update velocity chart
		velocityChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.alt_data.velocity });
		velocityChartRef.current.update('quiet');

		// Update acceleration chart
		accelerationChartRef.current.data.datasets[0].data.push({ x: time, y: received_data.acc_data.ax }); // ax
		accelerationChartRef.current.data.datasets[1].data.push({ x: time, y: received_data.acc_data.ay });
		accelerationChartRef.current.data.datasets[2].data.push({ x: time, y: received_data.acc_data.az });
		// accelerationChartRef.current.data.datasets[3].data.push({ x: time, y: received_data.filtered_a });
		accelerationChartRef.current.update('quiet');
	};


  return (

  <div className="h-full box-border m-0 text-black w-full mx-auto ">
    <main className="flex flex-col md:flex-row md:space-y-0 w-full h-screen selection:bg-blue-600 "> 
      <div className="md:w-1/5 h-screen">
        <Sidebar 
          state={state} 
          operationMode={operationMode} 
          altitude={altitude} 
          pressure={pressure}
          temperature={temperature}
          pyroDrogue={pyroDrogue}
          pyroMain={pyroMain}
          onConnect={handleConnect}
        />
      </div>
      <div className='w-full md:w-4/5'>
        <Header isConnected={isConnected} batteryVoltage={batteryVoltage} />
        <div className="mt-10 md:mt-16 grid grid-cols-1 md:grid-cols-2 gap-2 w-full p-2">

          <div className='flex flex-col space-y-2'>
              <Chart ref={altitudeChartRef} type="altitude" />
            
              <Chart ref={velocityChartRef} type="velocity" />
              
          </div>
          <div className='flex flex-col space-y-2'>  
          <div className="h-full w-full">
                <Video  />
            </div>

              <Chart ref={accelerationChartRef} type="acceleration" />
              
          </div>
        </div> 
        <div className="flex w-full p-2">
            <div className="h-[500px] w-[1555px] z-0 ">
              <Map position={[latitude, longitude]} />
            </div>
          </div>
   
        <Footer />
 
      </div>
  </main>

  </div>
  );
}

export default App;
