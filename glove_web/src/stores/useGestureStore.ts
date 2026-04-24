import { create } from 'zustand';
import type { SensorMessage, GestureHistoryEntry } from '../types';
import { GESTURE_LABELS } from '../utils/constants';

interface GestureState {
  // Current gesture results
  l1GestureId: number | null;
  l1Confidence: number | null;
  l2GestureId: number | null;
  l2Confidence: number | null;
  nlpText: string | null;

  // Gesture history (last N entries)
  gestureHistory: GestureHistoryEntry[];

  // Actions
  updateGesture: (data: SensorMessage) => void;
  clearHistory: () => void;
}

const MAX_HISTORY = 10;

function getGestureLabel(id: number | null): string {
  if (id === null || id === undefined) return '无手势';
  return GESTURE_LABELS[id] ?? `手势 #${id}`;
}

export const useGestureStore = create<GestureState>((set, get) => ({
  l1GestureId: null,
  l1Confidence: null,
  l2GestureId: null,
  l2Confidence: null,
  nlpText: null,
  gestureHistory: [],

  updateGesture: (data: SensorMessage) => {
    const prev = get();

    // Only add to history if gesture changed
    const gestureChanged = data.l1_gesture_id !== prev.l1GestureId;
    const shouldRecord =
      gestureChanged && data.l1_gesture_id !== null && (data.l1_confidence ?? 0) > 0.5;

    const newEntry: GestureHistoryEntry | null = shouldRecord
      ? {
          gestureId: data.l1_gesture_id,
          label: getGestureLabel(data.l1_gesture_id),
          confidence: data.l1_confidence ?? 0,
          nlpText: data.nlp_text,
          timestamp: data.timestamp,
        }
      : null;

    set({
      l1GestureId: data.l1_gesture_id,
      l1Confidence: data.l1_confidence,
      l2GestureId: data.l2_gesture_id,
      l2Confidence: data.l2_confidence,
      nlpText: data.nlp_text,
      gestureHistory: newEntry
        ? [newEntry, ...prev.gestureHistory].slice(0, MAX_HISTORY)
        : prev.gestureHistory,
    });
  },

  clearHistory: () => {
    set({ gestureHistory: [] });
  },
}));

// ── Convenience selectors ──
export const selectL1Label = (state: GestureState): string =>
  getGestureLabel(state.l1GestureId);

export const selectL2Label = (state: GestureState): string =>
  getGestureLabel(state.l2GestureId);
