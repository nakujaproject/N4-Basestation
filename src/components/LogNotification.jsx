import React from "react";
import { CircleAlert, TriangleAlert, Bug } from "lucide-react";

// Color and icon mapping for log levels
const LOG_LEVEL_STYLES = {
  INFO: {
    icon: CircleAlert,
    bgColor: "bg-blue-50",
    textColor: "text-blue-800",
    borderColor: "border-blue-200",
    iconColor: "text-blue-500",
  },
  WARN: {
    icon: TriangleAlert,
    bgColor: "bg-yellow-50",
    textColor: "text-yellow-800",
    borderColor: "border-yellow-200",
    iconColor: "text-yellow-500",
  },
  ERROR: {
    icon: TriangleAlert,
    bgColor: "bg-red-50",
    textColor: "text-red-800",
    borderColor: "border-red-200",
    iconColor: "text-red-500",
  },
  DEBUG: {
    icon: Bug,
    bgColor: "bg-purple-50",
    textColor: "text-purple-800",
    borderColor: "border-purple-200",
    iconColor: "text-purple-500",
  },
};

// Format timestamp for logs
const formatTimestamp = (timestamp) => {
  return new Date(timestamp).toLocaleString("en-US", {
    year: "numeric",
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
};

function LogNotification({ logs = [] }) {
  return (
    <div className="w-full p-2 rounded-2xl border-2 border-gray-700 flex flex-col">
      {/* Header */}
      <div className="text-xs uppercase w-fit bg-white px-1 py-0.5 rounded -mt-5 z-10">
        Logs {logs.length > 0 ? `(${logs.length})` : ""}
      </div>

      {/* Log container */}
      <div className="flex-1 overflow-y-auto max-h-64 mt-2">
        {logs.length === 0 && (
          <div className="text-xs text-gray-500 text-center p-2">
            No logs available
          </div>
        )}
        {logs.map((log, index) => {
          const levelStyle = LOG_LEVEL_STYLES[log.level] || LOG_LEVEL_STYLES.INFO;
          const LogLevelIcon = levelStyle.icon;

          return (
            <div
              key={index}
              className={`flex items-start space-x-2 p-2 border-b last:border-b-0 ${levelStyle.bgColor} ${levelStyle.borderColor}`}
            >
              <LogLevelIcon className={`h-4 w-4 mt-1 ${levelStyle.iconColor}`} />
              <div className="flex-1 text-xs">
                <div className="text-gray-600">{formatTimestamp(log.timestamp)}</div>
                <div className={`font-semibold ${levelStyle.textColor}`}>{log.level}</div>
                {log.action && log.status && (
                  <div>
                    {log.action} - {log.status}
                  </div>
                )}
                {log.message && <div>{log.message}</div>}
                {log.source && (
                  <div className="text-gray-400">Source: {log.source}</div>
                )}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}

export default LogNotification;
