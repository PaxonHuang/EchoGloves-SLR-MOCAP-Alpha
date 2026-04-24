import { useSettingsStore } from '../../stores/useSettingsStore';
import type { ConnectionStatus } from '../../types';

interface HeaderProps {
  connectionStatus: ConnectionStatus;
  fps: number;
}

// ── Connection status badge ──
function StatusBadge({ status }: { status: ConnectionStatus }) {
  const config = {
    connecting: { color: 'bg-yellow-500', pulse: true, label: '连接中' },
    connected: { color: 'bg-emerald-500', pulse: false, label: '已连接' },
    disconnected: { color: 'bg-slate-500', pulse: false, label: '未连接' },
    error: { color: 'bg-red-500', pulse: true, label: '错误' },
  }[status];

  return (
    <div className="flex items-center gap-2 rounded-full border border-slate-700 bg-slate-800/50 px-3 py-1">
      <div className="relative flex h-2 w-2">
        {config.pulse && (
          <span
            className={`absolute inline-flex h-full w-full animate-ping rounded-full opacity-75 ${config.color}`}
          />
        )}
        <span
          className={`relative inline-flex h-2 w-2 rounded-full ${config.color}`}
        />
      </div>
      <span className="text-xs font-medium text-slate-300">{config.label}</span>
    </div>
  );
}

export default function Header({ connectionStatus, fps }: HeaderProps) {
  const openSettings = useSettingsStore((s) => s.openSettings);

  return (
    <header className="flex h-14 shrink-0 items-center justify-between border-b border-slate-800 bg-slate-900/80 px-4 backdrop-blur-sm">
      {/* Left: Title */}
      <div className="flex items-center gap-3">
        <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-blue-500/20 text-sm">
          🧤
        </div>
        <div>
          <h1 className="text-sm font-bold tracking-tight text-slate-100">
            Edge AI 手语手套
          </h1>
          <p className="text-[10px] text-slate-500">Sign Language Recognition V3</p>
        </div>
      </div>

      {/* Right: Status + FPS + Settings */}
      <div className="flex items-center gap-3">
        {/* FPS Display */}
        <div className="hidden items-center gap-1.5 rounded-md bg-slate-800/50 px-2 py-1 sm:flex">
          <span className="text-[10px] text-slate-500">FPS</span>
          <span
            className={`font-mono text-xs font-bold ${
              fps >= 50 ? 'text-emerald-400' : fps >= 20 ? 'text-yellow-400' : 'text-red-400'
            }`}
          >
            {fps}
          </span>
        </div>

        {/* Connection Status */}
        <StatusBadge status={connectionStatus} />

        {/* Settings Button */}
        <button
          onClick={openSettings}
          className="flex h-8 w-8 items-center justify-center rounded-lg text-slate-400 transition-colors hover:bg-slate-800 hover:text-slate-200"
          aria-label="Settings"
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
              d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z"
            />
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z"
            />
          </svg>
        </button>
      </div>
    </header>
  );
}
