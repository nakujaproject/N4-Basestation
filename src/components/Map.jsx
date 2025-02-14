import {
  MapContainer,
  TileLayer,
  Marker,
  Popup,
  Circle,
  useMapEvent,
} from "react-leaflet";
import "leaflet/dist/leaflet.css"; // important

function CurrentPosition({ position }) {
  const map = useMapEvent("click", () => {
    map.setView(position, map.getZoom());
  });
  return position === null ? null : (
    <Marker position={position}>
      <Popup>Here I am!</Popup>
    </Marker>
  );
}

function Map({ position }) {
  // if ("geolocation" in navigator) {
  //     // Geolocation is available
  //     navigator.geolocation.getCurrentPosition(
  //       (position) => {
  //         const latitude = position.coords.latitude;
  //         const longitude = position.coords.longitude;
  //         console.log(`Your position is : ${latitude} ${longitude}`);
  //       },
  //       (error) => {
  //         console.log("Error getting location:", error.message);
  //       }
  //     );
  //   } else {
  //     // Geolocation is not available
  //     console.error("Geolocation is not supported by your browser.");
  //   }

  return (
    <MapContainer
      center={position}
      zoom={13}
      scrollWheelZoom={true}
      style={{ height: "100%", width: "100%" }}
    >
      <TileLayer
        attribution='<a href=\"https://www.maptiler.com/copyright/\" target=\"_blank\">&copy; MapTiler</a> | <a href=\"https://www.openstreetmap.org/copyright\" target=\"_blank\">&copy; OpenStreetMap contributors</a>'
        url="http://[::]:8080/styles/basic-preview/{z}/{x}/{y}.png"
      />
      <Circle center={position} pathOptions={{ fillColor: "blue" }} radius={20}>
        <Marker position={position}>
          <Popup>Rocket's Position</Popup>
        </Marker>
      </Circle>
      <CurrentPosition position={position} />
    </MapContainer>
  );
}

export default Map;
