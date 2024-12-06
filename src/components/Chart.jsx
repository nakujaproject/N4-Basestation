import { forwardRef, memo, useMemo } from "react";
import { Line } from "react-chartjs-2";
import "chartjs-adapter-luxon";
import StreamingPlugin from "@robloche/chartjs-plugin-streaming";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from "chart.js";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  StreamingPlugin
);

const Chart = forwardRef((props, ref) => {
  // if (!ref || !ref.current) {
  // 	console.warn('Chart reference is null.');
  // 	return null;
  //   }

  let ylabel = "";

  let dataset = [{}];

  switch (props.type) {
    case "altitude":
      ylabel = "Altitude(m)";
      dataset = [
        {
          label: "Altitude",
          backgroundColor: "rgba(54, 162, 235, 0.5)",
          borderColor: "rgb(54, 162, 235)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
        {
          label: "AGL",
          backgroundColor: "rgba(154, 2, 25, 0.5)",
          borderColor: "rgb(154, 16, 23)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
      ];
      break;
    case "velocity":
      ylabel = "Velocity (m/s)";
      dataset = [
        {
          label: "Velocity",
          backgroundColor: "rgba(255, 99, 132, 0.5)",
          borderColor: "rgb(1, 99, 132)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
      ];
      break;
    case "acceleration":
      ylabel = "Acceleration (m/s²)";

      dataset = [
        {
          label: "ax",
          backgroundColor: "rgba(54, 162, 235, 0.5)",
          borderColor: "rgb(54, 162, 235)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
        {
          label: "ay",
          borderColor: "rgb(255,165,0)",
          backgroundColor: "rgb(255,165,0,0.5)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
        {
          label: "az",
          borderColor: "rgb(60,186,159)",
          backgroundColor: "rgb(60,186,159,0.5)",
          cubicInterpolationMode: "monotone",
          data: [],
        },
      ];
      break;
    default:
      ylabel = "Altitude (m)";
  }

  const data = { datasets: dataset };

  const options = {
    responsive: true,
    animation: false,
    datasets: {
      line: {
        borderWidth: 1,
        pointRadius: 1,
      },
    },
    scales: {
      x: {
        type: "realtime",
        realtime: {
          delay: 1000,
          pause: false,
          ttl: 10000,
          duration: 10000,
          frameRate: 60,
        },
        ticks: {
          font: {
            size: 12,
            weight: "bolder",
          },
          color: "#000",
        },
        title: {
          display: false,
          text: "Time",
          font: {
            size: 12,
            weight: "bolder",
          },
          color: "#000",
        },
      },
      y: {
        ticks: {
          font: {
            size: 12,
            weight: "bolder",
          },
          color: "#000",
        },
        title: {
          display: true,
          text: ylabel,
          font: {
            size: 12,
            weight: "bolder",
          },
          color: "#000",
        },
      },
    },
    plugins: {
      // streaming :{
      //     frameRate: 1
      // },
      legend: {
        position: "top",
        align: "end",
      },
      title: {
        display: false,
        text: "Telemetry Graph",
      },
    },
  };

  return (
    <Line
      ref={ref}
      data={data}
      options={options}
      height={170}
      className="shadow-md flex flex-grow w-full bg-gray-200 px-2"
    />
  );
});

export default memo(Chart);
