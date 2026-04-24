import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { AppSettings, Language } from '../types';

interface SettingsState extends AppSettings {
  // Panel visibility
  settingsOpen: boolean;

  // Actions
  setRelayHost: (host: string) => void;
  toggle3D: () => void;
  toggleDashboard: () => void;
  toggleDarkMode: () => void;
  setLanguage: (lang: Language) => void;
  openSettings: () => void;
  closeSettings: () => void;
}

const STORAGE_KEY = 'glove-web-settings';

export const useSettingsStore = create<SettingsState>()(
  persist(
    (set) => ({
      relayHost: 'localhost',
      show3D: true,
      showDashboard: true,
      darkMode: true,
      language: 'zh' as Language,
      settingsOpen: false,

      setRelayHost: (host: string) => set({ relayHost: host }),
      toggle3D: () => set((s) => ({ show3D: !s.show3D })),
      toggleDashboard: () => set((s) => ({ showDashboard: !s.showDashboard })),
      toggleDarkMode: () => set((s) => ({ darkMode: !s.darkMode })),
      setLanguage: (lang: Language) => set({ language: lang }),
      openSettings: () => set({ settingsOpen: true }),
      closeSettings: () => set({ settingsOpen: false }),
    }),
    {
      name: STORAGE_KEY,
      // Only persist these fields to localStorage
      partialize: (state) => ({
        relayHost: state.relayHost,
        show3D: state.show3D,
        showDashboard: state.showDashboard,
        darkMode: state.darkMode,
        language: state.language,
      }),
    },
  ),
);
