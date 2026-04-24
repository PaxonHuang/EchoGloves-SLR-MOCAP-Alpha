import { Suspense, lazy } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { useSensorStore } from './stores/useSensorStore';
import { useSettingsStore } from './stores/useSettingsStore';
import Header from './components/Layout/Header';
import MobileNav from './components/Layout/MobileNav';
import GestureResult from './components/Dashboard/GestureResult';
import SensorDataPanel from './components/Dashboard/SensorDataPanel';
import StatsPanel from './components/Dashboard/StatsPanel';
import SettingsSidebar from './components/Settings/SettingsSidebar';

const HandCanvas = lazy(() =>
  import('./components/Hand3D/HandCanvas').then((m) => ({ default: m.HandCanvas })),
);

export default function App() {
  const { status } = useWebSocket();
  const fps = useSensorStore((s) => s.fps);
  const isStreaming = useSensorStore((s) => s.isStreaming);
  const { show3D, showDashboard, darkMode } = useSettingsStore();

  return (
    <div
      className={`flex min-h-screen flex-col ${
        darkMode ? 'dark bg-slate-950 text-slate-100' : 'bg-white text-slate-900'
      }`}
    >
      {/* ── Header ── */}
      <Header connectionStatus={status} fps={fps} />

      {/* ── Main Content ── */}
      <main className="flex flex-1 flex-col overflow-hidden lg:flex-row">
        {/* 3D Hand Visualization */}
        {show3D && (
          <section className="relative order-1 min-h-[320px] flex-1 lg:order-none">
            <Suspense
              fallback={
                <div className="flex h-full items-center justify-center bg-slate-900">
                  <div className="animate-pulse text-slate-400">Loading 3D Hand...</div>
                </div>
              }
            >
              <HandCanvas />
            </Suspense>
            {!isStreaming && status === 'connected' && (
              <div className="absolute inset-0 flex items-center justify-center bg-slate-950/60">
                <p className="text-sm text-slate-400">Waiting for sensor data...</p>
              </div>
            )}
          </section>
        )}

        {/* Dashboard Panels */}
        {showDashboard && (
          <aside className="order-2 flex flex-col gap-4 overflow-y-auto border-t border-slate-800 p-4 lg:order-none lg:w-[420px] lg:border-l lg:border-t-0">
            <GestureResult />
            <SensorDataPanel />
            <StatsPanel connectionStatus={status} />
          </aside>
        )}
      </main>

      {/* ── Mobile Navigation ── */}
      <MobileNav />

      {/* ── Settings Sidebar (rendered via portal or inline) ── */}
      <SettingsSidebar />
    </div>
  );
}
