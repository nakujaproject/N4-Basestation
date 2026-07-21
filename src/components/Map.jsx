import React, { useState, useEffect, useRef } from "react";
import {
  MapContainer,
  Marker,
  Popup,
  Polyline,
  useMap,
  useMapEvent,
  TileLayer,
} from "react-leaflet";
import "leaflet/dist/leaflet.css";
import L from "leaflet";

// Fix for default markers in react-leaflet
delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon-2x.png',
  iconUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
});

// Custom icons
const currentPositionIcon = new L.Icon({
  iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-blue.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
  iconSize: [25, 41],
  iconAnchor: [12, 41],
  popupAnchor: [1, -34],
  shadowSize: [41, 41]
});

const rocketIcon = new L.Icon({
  iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-red.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
  iconSize: [25, 41],
  iconAnchor: [12, 41],
  popupAnchor: [1, -34],
  shadowSize: [41, 41]
});

// Fallback to standard TileLayer
function MapTileLayer() {
  return (
    <TileLayer
      attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
      url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
    />
  );
}

// Safe coordinate display function
function formatCoordinate(coord) {
  if (coord === undefined || coord === null) return 'N/A';
  if (typeof coord === 'number') return coord.toFixed(6);
  return 'N/A';
}

// Movable pin component with proper error handling
function MovablePin({ position, onPositionChange }) {
  const [pinPosition, setPinPosition] = useState(position || { lat: 0, lng: 0 });
  const markerRef = useRef();

  useEffect(() => {
    if (position && position.lat !== undefined && position.lng !== undefined) {
      setPinPosition(position);
    }
  }, [position]);

  const eventHandlers = {
    dragend() {
      const marker = markerRef.current;
      if (marker != null) {
        const newPosition = marker.getLatLng();
        setPinPosition(newPosition);
        if (onPositionChange) {
          onPositionChange(newPosition);
        }
      }
    },
  };

  // Don't render if position is invalid
  if (!pinPosition || pinPosition.lat === undefined || pinPosition.lng === undefined) {
    return null;
  }

  return (
    <Marker
      draggable={true}
      eventHandlers={eventHandlers}
      position={pinPosition}
      ref={markerRef}
      icon={currentPositionIcon}
    >
      <Popup>
        <div className="text-center">
          <strong>Movable Pin</strong>
          <br />
          Drag me to new location
          <br />
          Lat: {formatCoordinate(pinPosition.lat)}
          <br />
          Lng: {formatCoordinate(pinPosition.lng)}
        </div>
      </Popup>
    </Marker>
  );
}

// Center to pin button
function CenterToPinButton({ targetPosition }) {
  const map = useMap();
  const buttonRef = useRef();

  useEffect(() => {
    if (buttonRef.current) {
      L.DomEvent.disableClickPropagation(buttonRef.current);
    }
  }, []);

  const centerToPin = () => {
    if (targetPosition && targetPosition.lat !== undefined && targetPosition.lng !== undefined) {
      map.setView(targetPosition, map.getZoom());
    }
  };

  return (
    <div className="leaflet-top leaflet-right">
      <div className="leaflet-control">
        <button
          ref={buttonRef}
          onClick={centerToPin}
          className="bg-white hover:bg-gray-100 text-gray-800 font-semibold py-2 px-4 border border-gray-400 rounded shadow text-sm"
          style={{ margin: '10px' }}
        >
          📍 Center to Pin
        </button>
      </div>
    </div>
  );
}

// Geolocation button
function GeolocationButton({ onLocationFound }) {
  const map = useMap();
  const buttonRef = useRef();

  useEffect(() => {
    if (buttonRef.current) {
      L.DomEvent.disableClickPropagation(buttonRef.current);
    }
  }, []);

  const getCurrentLocation = () => {
    if (navigator.geolocation) {
      navigator.geolocation.getCurrentPosition(
        (position) => {
          const { latitude, longitude } = position.coords;
          const userLocation = L.latLng(latitude, longitude);
          
          map.setView(userLocation, 16);
          if (onLocationFound) {
            onLocationFound(userLocation);
          }
        },
        (error) => {
          console.error("Error getting location:", error);
          alert("Unable to get your current location. Please check location permissions.");
        },
        {
          enableHighAccuracy: true,
          timeout: 10000,
          maximumAge: 60000
        }
      );
    } else {
      alert("Geolocation is not supported by this browser.");
    }
  };

  return (
    <div className="leaflet-top leaflet-right" style={{ top: '50px' }}>
      <div className="leaflet-control">
        <button
          ref={buttonRef}
          onClick={getCurrentLocation}
          className="bg-blue-500 hover:bg-blue-600 text-white font-semibold py-2 px-4 border border-blue-600 rounded shadow text-sm"
          style={{ margin: '10px' }}
        >
          🎯 Find My Location
        </button>
      </div>
    </div>
  );
}

// Click to set position
function ClickToSetPosition({ onPositionSet }) {
  const map = useMapEvent("click", (e) => {
    if (onPositionSet) {
      onPositionSet(e.latlng);
    }
  });
  return null;
}

// Safe marker component
function SafeMarker({ position, icon, children }) {
  if (!position || position.lat === undefined || position.lng === undefined) {
    return null;
  }
  return (
    <Marker position={position} icon={icon}>
      {children}
    </Marker>
  );
}

function Map({ position, path, onPinPositionChange }) {
  // Ensure we have a valid default position
  const defaultPosition = position && position.lat !== undefined && position.lng !== undefined 
    ? position 
    : { lat: -1.2921, lng: 36.8219 }; // Nairobi coordinates
    
  const [pinPosition, setPinPosition] = useState(defaultPosition);
  const [userLocation, setUserLocation] = useState(null);

  const start = path && path.length > 0 ? path[0] : null;
  const end = path && path.length > 0 ? path[path.length - 1] : null;

  const handlePinPositionChange = (newPosition) => {
    setPinPosition(newPosition);
    if (onPinPositionChange) {
      onPinPositionChange(newPosition);
    }
  };

  const handleLocationFound = (location) => {
    setUserLocation(location);
    setPinPosition(location);
    if (onPinPositionChange) {
      onPinPositionChange(location);
    }
  };

  const handleMapClick = (latlng) => {
    setPinPosition(latlng);
    if (onPinPositionChange) {
      onPinPositionChange(latlng);
    }
  };

  return (
    <MapContainer
      center={defaultPosition}
      zoom={13}
      scrollWheelZoom={true}
      style={{ height: "100%", width: "100%" }}
    >
      {/* Use standard tile layer */}
      <MapTileLayer />
      
      {/* Control Buttons */}
      <CenterToPinButton targetPosition={pinPosition} />
      <GeolocationButton onLocationFound={handleLocationFound} />
      
      {/* Click to set position */}
      <ClickToSetPosition onPositionSet={handleMapClick} />

      {/* Flight path */}
      {path && path.length > 1 && (
        <Polyline 
          positions={path} 
          color="#3b82f6" 
          weight={4}
          opacity={0.7}
        />
      )}
      
      {/* Start marker */}
      {start && (
        <SafeMarker position={start} icon={currentPositionIcon}>
          <Popup>
            <strong>Start Position</strong>
            <br />
            Lat: {formatCoordinate(start.lat)}
            <br />
            Lng: {formatCoordinate(start.lng)}
          </Popup>
        </SafeMarker>
      )}
      
      {/* End marker (rocket location) */}
      {end && (
        <SafeMarker position={end} icon={rocketIcon}>
          <Popup>
            <strong>🚀 Rocket Location</strong>
            <br />
            Lat: {formatCoordinate(end.lat)}
            <br />
            Lng: {formatCoordinate(end.lng)}
          </Popup>
        </SafeMarker>
      )}
      
      {/* User location marker if found */}
      {userLocation && (
        <SafeMarker position={userLocation} icon={currentPositionIcon}>
          <Popup>
            <strong>📍 Your Current Location</strong>
            <br />
            Automatically detected
          </Popup>
        </SafeMarker>
      )}
      
      {/* Movable pin */}
      <MovablePin 
        position={pinPosition} 
        onPositionChange={handlePinPositionChange} 
      />
    </MapContainer>
  );
}

export default Map;