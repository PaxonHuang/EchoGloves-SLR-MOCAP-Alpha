// ── Sensor Message from Python Relay (WebSocket) ──
export interface SensorMessage {
  timestamp: number;
  hall: number[];          // 15 floats (flex sensor values)
  imu: number[];           // 6 floats [gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z]
  l1_gesture_id: number | null;
  l1_confidence: number | null;
  l2_gesture_id: number | null;
  l2_confidence: number | null;
  nlp_text: string | null;
  status: 'STREAMING' | 'MODEL_SWITCHING' | 'ERROR';
}

// ── 3D Hand Pose ──
export interface Keypoint3D {
  x: number;
  y: number;
  z: number;
}

export interface HandPose {
  keypoints: Keypoint3D[];           // 21 keypoints
  quaternion: [number, number, number, number]; // [w, x, y, z]
}

// ── Connection Status ──
export type ConnectionStatus = 'connecting' | 'connected' | 'disconnected' | 'error';

// ── Gesture History Entry ──
export interface GestureHistoryEntry {
  gestureId: number | null;
  label: string;
  confidence: number;
  nlpText: string | null;
  timestamp: number;
}

// ── App Settings ──
export type Language = 'zh' | 'en';

export interface AppSettings {
  relayHost: string;
  show3D: boolean;
  showDashboard: boolean;
  darkMode: boolean;
  language: Language;
}
