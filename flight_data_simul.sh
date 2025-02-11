#!/bin/bash

# MQTT Broker settings
BROKER_HOST="localhost"
BROKER_PORT=1882
TOPIC="n4/telemetry"

# Function to generate random float within a specified range
generate_random_float() {
    echo "scale=2; $1 + ($2 - $1) * $RANDOM / 32767" | bc
}

# Function to select a random state from [0, 1, 2, 3, 4, 5, 6]
generate_random_state() {
    echo $((RANDOM % 7))  # Generates a random number from 0 to 6
}

# Function to select a random state from [0, 1]
generate_random_op() {
    echo $((RANDOM % 2))  # Generates a random number between 0 and 1
}

# Simulate the telemetry data
while true; do
    # Simulate telemetry values
    id=0
    state=$(generate_random_state)
    operation_mode=$(generate_random_op)
    ax=$(generate_random_float 1 10)
    ay=$(generate_random_float 1 10)
    az=$(generate_random_float 1 10)
    pitch=$(generate_random_float 1 90)
    roll=$(generate_random_float 1 180)
    gx=$(generate_random_float 1 10)
    gy=$(generate_random_float 1 10)
    gz=$(generate_random_float 1 10)
    latitude=$(generate_random_float 1 90)
    longitude=$(generate_random_float 1 180)
    gps_altitude=$(generate_random_float 1 3000)
    time=$(date +%s)  # Current Unix timestamp
    pressure=$(generate_random_float 900 1100)  # Altimeter pressure in hPa
    temperature=$(generate_random_float 1 50)  # Temperature in °C
    AGL=$(generate_random_float 1 3000)  # Above Ground Level
    velocity=$(generate_random_float 1 200)  # Velocity in m/s
    pyro1_state=$(generate_random_op)
    pyro2_state=$(generate_random_op)
    battery_voltage=$(generate_random_float 1 15)  # Battery voltage in Volts

    # Construct the JSON message
    json_message=$(cat <<EOF
{
  "id": $id,
  "state": $state,
  "operation_mode": $operation_mode,
  "acc_data": {
    "ax": $ax,
    "ay": $ay,
    "az": $az,
    "pitch": $pitch,
    "roll": $roll
  },
  "gyro_data": {
    "gx": $gx,
    "gy": $gy,
    "gz": $gz
  },
  "gps_data": {
    "latitude": $latitude,
    "longitude": $longitude,
    "gps_altitude": $gps_altitude,
    "time": $time
  },
  "alt_data": {
    "pressure": $pressure,
    "temperature": $temperature,
    "AGL": $AGL,
    "velocity": $velocity
  },
  "chute_state": {
    "pyro1_state": $pyro1_state,
    "pyro2_state": $pyro2_state
  },
  "battery_voltage": $battery_voltage
}
EOF
)

    # Send the data via MQTT
    mosquitto_pub -h "$BROKER_HOST" -p "$BROKER_PORT" -t "$TOPIC" -m "$json_message"

    # Sleep for a period to simulate real-time data
    sleep 0.001  # Change this value for higher or lower frequency
done

