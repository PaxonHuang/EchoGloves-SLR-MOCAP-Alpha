import { create } from 'zustand';
import type { SensorMessage } from '../types';

interface SensorState {
  // Current sensor values
  hall: number[];
  imu: number[];
  timestamp: number;

  // Streaming state
  isStreaming: boolean;

  // Performance metrics
  fps: number;
  packetCount: number;
  lastPacketTime: number;

  // Actions
  updateFromRelay: (data: SensorMessage) => void;
  reset: () => void;
}

const INITIAL_STATE = {
  hall: new Array(15).fill(0) as number[],
  imu: new Array(6).fill(0) as number[],
  timestamp: 0,
  isStreaming: false,
  fps: 0,
  packetCount: 0,
  lastPacketTime: 0,
};

export const useSensorStore = create<SensorState>((set, get) => ({
  ...INITIAL_STATE,

  updateFromRelay: (data: SensorMessage) => {
    const now = performance.now();
    const prev = get();

    // Calculate FPS based on packet intervals
    const newPacketCount = prev.packetCount + 1;
    let fps = prev.fps;
    if (prev.lastPacketTime > 0) {
      const interval = now - prev.lastPacketTime;
      const instantFps = interval > 0 ? 1000 / interval : 0;
      fps = Math.round(fps * 0.7 + instantFps * 0.3); // EMA smoothing
    }

    set({
      hall: data.hall,
      imu: data.imu,
      timestamp: data.timestamp,
      isStreaming: data.status === 'STREAMING',
      fps,
      packetCount: newPacketCount,
      lastPacketTime: now,
    });
  },

  reset: () => {
    set(INITIAL_STATE);
  },
}));
