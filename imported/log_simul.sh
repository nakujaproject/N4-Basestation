#!/bin/bash

# MQTT Broker Details
BROKER_HOST="localhost"
BROKER_PORT=1882
TOPIC="n4/logs"

# Function to generate a random log level
get_log_level() {
    local levels=("INFO" "WARN" "ERROR" "DEBUG")
    echo "${levels[RANDOM % ${#levels[@]}]}"
}

# Function to generate a sample log message
get_log_message() {
    local messages=(
        "System initialization complete"
        "Temperature sensor reading normal"
        "Network connection established"
        "Battery voltage check passed"
        "Gyroscope calibration in progress"
        "Memory diagnostic completed"
        "Unexpected sensor deviation detected"
        "Thermal regulation active"
    )
    echo "${messages[RANDOM % ${#messages[@]}]}"
}

# Send log messages in a loop
while true; do
    # Prepare JSON payload
    PAYLOAD=$(jq -n \
        --arg level "$(get_log_level)" \
        --arg message "$(get_log_message)" \
        --arg source "Flight Computer" \
        '{level: $level, message: $message, source: $source}')

    # Publish message to MQTT broker
    mosquitto_pub -h "$BROKER_HOST" -p "$BROKER_PORT" -t "$TOPIC" -m "$PAYLOAD"

    # Wait for a random interval between 1-5 seconds
    sleep 0.05
done