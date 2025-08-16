import { useMemo } from 'react';
import throttle from 'lodash.throttle';

// Create throttled setter utilities
export const createThrottledSetter = (setter, delay) => {
  return useMemo(
    () =>
      throttle((val) => {
        setter(val);
      }, delay),
    [setter, delay]
  );
};

// Create altitude-based antenna logic
export const determineAntenna = (altitude) => {
  if (altitude < 1000) return 'A-1';
  if (altitude >= 1000 && altitude < 2000) return 'A-2';
  if (altitude >= 2000) return 'A-3';
};

// Create state-to-status mapping logic
export const getRocketStatusAndColor = (state) => {
  switch (state) {
    case 0:
      return { status: 'Pre Flight', color: 'text-gray-500' };
    case 1:
      return { status: 'Powered Flight', color: 'text-purple-500' };
    case 2:
      return { status: 'Apogee', color: 'text-red-500' };
    case 3:
      return { status: 'Drogue Deployed', color: 'text-orange-500' };
    case 4:
      return { status: 'Main Deployed', color: 'text-yellow-500' };
    case 5:
      return { status: 'Rocket Descent', color: 'text-blue-500' };
    case 6:
      return { status: 'Post Flight', color: 'text-green-500' };
    default:
      return { status: 'Pre Flight', color: 'text-blue-500' };
  }
};
