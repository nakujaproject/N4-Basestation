import serial
from flask import Flask, jsonify
from flask_cors import CORS
import json
import logging
from serial.tools import list_ports

app = Flask(__name__)
CORS(app)

# Set up logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

def find_arduino_port():
    """Find the Arduino serial port automatically."""
    ports = list_ports.comports()
    for port in ports:
        # Common Arduino identifiers
        if "Arduino" in port.description or "CH340" in port.description or "USB" in port.description:
            return port.device
    return None

def initialize_serial():
    """Initialize serial connection with error handling."""
    try:
        # Try to find Arduino port automatically
        port = find_arduino_port() or '/dev/ttyUSB0'  # Fallback to default
        ser = serial.Serial(port, 115200, timeout=None)
        logger.info(f"Successfully connected to {port}")
        return ser
    except serial.SerialException as e:
        logger.error(f"Failed to connect to serial port: {e}")
        return None

# Initialize serial connection
serial_connection = initialize_serial()

@app.route('/debug', methods=['GET'])
def debug_info():
    """Endpoint to get debug information about the serial connection."""
    if serial_connection is None:
        return jsonify({
            "status": "error",
            "message": "No serial connection",
            "available_ports": [port.device for port in list_ports.comports()]
        })
    
    return jsonify({
        "status": "connected",
        "port": serial_connection.port,
        "baudrate": serial_connection.baudrate,
        "is_open": serial_connection.is_open,
        "in_waiting": serial_connection.in_waiting if serial_connection.is_open else 0
    })

@app.route('/data', methods=['GET'])
def get_data():
    """Get data from serial port with enhanced error handling."""
    if serial_connection is None:
        logger.error("No serial connection available")
        return jsonify({"error": "Serial connection not available"}), 503
    
    try:
        if not serial_connection.is_open:
            serial_connection.open()
        
        if serial_connection.in_waiting:
            raw_data = serial_connection.readline().decode('utf-8').strip()
            logger.debug(f"Received raw data: {raw_data}")
            
            try:
                parsed_data = json.loads(raw_data)
                return jsonify(parsed_data)
            except json.JSONDecodeError as e:
                logger.error(f"JSON parsing error: {e}")
                return jsonify({"error": f"Invalid JSON data: {str(e)}"}), 400
        else:
            logger.debug("No data available in serial buffer")
            return jsonify({"error": "No data available in serial buffer"}), 404
            
    except serial.SerialException as e:
        logger.error(f"Serial port error: {e}")
        return jsonify({"error": f"Serial port error: {str(e)}"}), 500
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        return jsonify({"error": f"Unexpected error: {str(e)}"}), 500

@app.route('/reconnect', methods=['GET'])
def reconnect():
    """Endpoint to force a reconnection to the serial port."""
    global serial_connection
    
    try:
        if serial_connection and serial_connection.is_open:
            serial_connection.close()
        
        serial_connection = initialize_serial()
        
        if serial_connection and serial_connection.is_open:
            return jsonify({"status": "success", "message": f"Reconnected to {serial_connection.port}"})
        else:
            return jsonify({"status": "error", "message": "Failed to reconnect"}), 500
            
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    app.run(host='localhost', port=5000, debug=True)