import { useEffect, useRef } from 'react';
import { useSettingsStore } from '../../stores/useSettingsStore';

export default function SettingsSidebar() {
  const settingsOpen = useSettingsStore((s) => s.settingsOpen);
  const closeSettings = useSettingsStore((s) => s.closeSettings);
  const relayHost = useSettingsStore((s) => s.relayHost);
  const setRelayHost = useSettingsStore((s) => s.setRelayHost);
  const show3D = useSettingsStore((s) => s.show3D);
  const toggle3D = useSettingsStore((s) => s.toggle3D);
  const showDashboard = useSettingsStore((s) => s.showDashboard);
  const toggleDashboard = useSettingsStore((s) => s.toggleDashboard);
  const darkMode = useSettingsStore((s) => s.darkMode);
  const toggleDarkMode = useSettingsStore((s) => s.toggleDarkMode);
  const language = useSettingsStore((s) => s.language);
  const setLanguage = useSettingsStore((s) => s.setLanguage);

  const overlayRef = useRef<HTMLDivElement>(null);

  // Close on Escape key
  useEffect(() => {
    if (!settingsOpen) return;

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') closeSettings();
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [settingsOpen, closeSettings]);

  // Close when clicking overlay
  const handleOverlayClick = (e: React.MouseEvent) => {
    if (e.target === overlayRef.current) {
      closeSettings();
    }
  };

  if (!settingsOpen) return null;

  return (
    <div
      ref={overlayRef}
      onClick={handleOverlayClick}
      className="fixed inset-0 z-50 flex items-center justify-end bg-black/50 backdrop-blur-sm"
    >
      {/* Settings Panel */}
      <aside className="flex h-full w-80 max-w-[85vw] flex-col overflow-y-auto border-l border-slate-700 bg-slate-900 p-6 shadow-2xl animate-fade-in">
        {/* Header */}
        <div className="mb-6 flex items-center justify-between">
          <h2 className="text-lg font-bold text-slate-100">设置</h2>
          <button
            onClick={closeSettings}
            className="flex h-8 w-8 items-center justify-center rounded-lg text-slate-400 transition-colors hover:bg-slate-800 hover:text-slate-200"
            aria-label="Close settings"
          >
            <svg
              xmlns="http://www.w3.org/2000/svg"
              className="h-5 w-5"
              fill="none"
              viewBox="0 0 24 24"
              stroke="currentColor"
              strokeWidth={1.5}
            >
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                d="M6 18 18 6M6 6l12 12"
              />
            </svg>
          </button>
        </div>

        {/* Settings Sections */}
        <div className="space-y-6">
          {/* ── Relay Connection ── */}
          <section>
            <h3 className="mb-3 text-xs font-semibold uppercase tracking-wider text-slate-500">
              中继服务器
            </h3>
            <div className="space-y-3">
              <div>
                <label
                  htmlFor="relay-host"
                  className="mb-1 block text-xs text-slate-400"
                >
                  Host 地址
                </label>
                <input
                  id="relay-host"
                  type="text"
                  value={relayHost}
                  onChange={(e) => setRelayHost(e.target.value)}
                  placeholder="localhost"
                  className="w-full rounded-md border border-slate-700 bg-slate-800 px-3 py-2 text-sm text-slate-200 placeholder-slate-600 outline-none transition-colors focus:border-blue-500 focus:ring-1 focus:ring-blue-500/30"
                />
                <p className="mt-1 text-[10px] text-slate-600">
                  WebSocket 连接: ws://{relayHost}:8765
                </p>
              </div>
            </div>
          </section>

          {/* ── Display ── */}
          <section>
            <h3 className="mb-3 text-xs font-semibold uppercase tracking-wider text-slate-500">
              显示选项
            </h3>
            <div className="space-y-3">
              {/* 3D Visualization Toggle */}
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm text-slate-300">3D 手型可视化</p>
                  <p className="text-[10px] text-slate-600">
                    React Three Fiber 手骨架
                  </p>
                </div>
                <button
                  onClick={toggle3D}
                  className={`relative h-6 w-11 rounded-full transition-colors ${
                    show3D ? 'bg-blue-500' : 'bg-slate-700'
                  }`}
                  role="switch"
                  aria-checked={show3D}
                >
                  <span
                    className={`absolute top-0.5 left-0.5 h-5 w-5 rounded-full bg-white shadow transition-transform ${
                      show3D ? 'translate-x-5' : 'translate-x-0'
                    }`}
                  />
                </button>
              </div>

              {/* Dashboard Toggle */}
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm text-slate-300">仪表板面板</p>
                  <p className="text-[10px] text-slate-600">
                    传感器数据、手势识别、统计
                  </p>
                </div>
                <button
                  onClick={toggleDashboard}
                  className={`relative h-6 w-11 rounded-full transition-colors ${
                    showDashboard ? 'bg-blue-500' : 'bg-slate-700'
                  }`}
                  role="switch"
                  aria-checked={showDashboard}
                >
                  <span
                    className={`absolute top-0.5 left-0.5 h-5 w-5 rounded-full bg-white shadow transition-transform ${
                      showDashboard ? 'translate-x-5' : 'translate-x-0'
                    }`}
                  />
                </button>
              </div>

              {/* Dark Mode Toggle */}
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm text-slate-300">深色模式</p>
                  <p className="text-[10px] text-slate-600">
                    暗色 / 亮色主题切换
                  </p>
                </div>
                <button
                  onClick={toggleDarkMode}
                  className={`relative h-6 w-11 rounded-full transition-colors ${
                    darkMode ? 'bg-blue-500' : 'bg-slate-700'
                  }`}
                  role="switch"
                  aria-checked={darkMode}
                >
                  <span
                    className={`absolute top-0.5 left-0.5 h-5 w-5 rounded-full bg-white shadow transition-transform ${
                      darkMode ? 'translate-x-5' : 'translate-x-0'
                    }`}
                  />
                </button>
              </div>
            </div>
          </section>

          {/* ── Language ── */}
          <section>
            <h3 className="mb-3 text-xs font-semibold uppercase tracking-wider text-slate-500">
              语言
            </h3>
            <div className="grid grid-cols-2 gap-2">
              <button
                onClick={() => setLanguage('zh')}
                className={`rounded-md border px-3 py-2 text-sm font-medium transition-colors ${
                  language === 'zh'
                    ? 'border-blue-500 bg-blue-500/20 text-blue-400'
                    : 'border-slate-700 bg-slate-800 text-slate-400 hover:border-slate-600'
                }`}
              >
                🇨🇳 中文
              </button>
              <button
                onClick={() => setLanguage('en')}
                className={`rounded-md border px-3 py-2 text-sm font-medium transition-colors ${
                  language === 'en'
                    ? 'border-blue-500 bg-blue-500/20 text-blue-400'
                    : 'border-slate-700 bg-slate-800 text-slate-400 hover:border-slate-600'
                }`}
              >
                🇺🇸 English
              </button>
            </div>
          </section>

          {/* ── About ── */}
          <section className="border-t border-slate-800 pt-4">
            <div className="space-y-1 text-center">
              <p className="text-xs text-slate-500">Edge AI 手语手套</p>
              <p className="text-[10px] text-slate-600">Version 3.0.0</p>
              <p className="text-[10px] text-slate-600">
                React 18 + R3F + Zustand + TailwindCSS v4
              </p>
            </div>
          </section>
        </div>
      </aside>
    </div>
  );
}
