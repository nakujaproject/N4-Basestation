import React, { useState } from "react";
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
  const [isOpen, setIsOpen] = useState(false);

  const toggleDropdown = () => setIsOpen(!isOpen);

  return (
    <div className="w-full p-2 rounded-2xl border-2 border-gray-700 relative">
      <div
        className="text-xs uppercase -mt-5 w-fit bg-white px-1 z-10 cursor-pointer"
        onClick={toggleDropdown}
      >
        Logs {logs.length > 0 ? `(${logs.length})` : ""}
      </div>
      {isOpen && (
        <div className="max-h-48 overflow-y-auto mt-2">
          {logs.map((log, index) => {
            const levelStyle =
              LOG_LEVEL_STYLES[log.level] || LOG_LEVEL_STYLES.INFO;
            const LogLevelIcon = levelStyle.icon;
            return (
              <div
                key={index}
                className={`text-xs p-1 border-b last:border-b-0 flex items-start space-x-3 ${levelStyle.bgColor} ${levelStyle.borderColor}`}
              >
                <LogLevelIcon
                  className={`h-4 w-4 mt-1 ${levelStyle.iconColor}`}
                />
                <div>
                  <div>{formatTimestamp(log.timestamp)}</div>
                  <div className="font-semibold">{log.level}</div>
                  {log.action && (
                    <div>
                      {log.action} - {log.status}
                    </div>
                  )}
                  {log.message && <div>{log.message}</div>}
                  {log.source && (
                    <div className="text-xs text-gray-500">
                      Source: {log.source}
                    </div>
                  )}
                </div>
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
}

export default LogNotification;
