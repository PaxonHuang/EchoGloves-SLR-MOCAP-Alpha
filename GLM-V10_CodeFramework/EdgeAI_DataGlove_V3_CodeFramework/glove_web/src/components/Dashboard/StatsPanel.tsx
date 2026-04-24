import { useState, useEffect } from 'react';
import { useSensorStore } from '../../stores/useSensorStore';
import type { ConnectionStatus } from '../../types';

interface StatsPanelProps {
  connectionStatus: ConnectionStatus;
}

export default function StatsPanel({ connectionStatus }: StatsPanelProps) {
  const fps = useSensorStore((s) => s.fps);
  const packetCount = useSensorStore((s) => s.packetCount);
  const isStreaming = useSensorStore((s) => s.isStreaming);

  const [uptime, setUptime] = useState(0);
  const [startTime] = useState(() => Date.now());

  // Update uptime every second
  useEffect(() => {
    if (connectionStatus !== 'connected') {
      setUptime(0);
      return;
    }

    const interval = setInterval(() => {
      setUptime(Math.floor((Date.now() - startTime) / 1000));
    }, 1000);

    return () => clearInterval(interval);
  }, [connectionStatus, startTime]);

  // Format uptime as HH:MM:SS
  const formatUptime = (seconds: number): string => {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  };

  // FPS color
  const fpsColor =
    fps >= 50
      ? 'text-emerald-400'
      : fps >= 20
        ? 'text-yellow-400'
        : 'text-red-400';

  return (
    <div className="animate-fade-in rounded-lg border border-slate-800 bg-slate-900/50 p-4">
      <h3 className="mb-3 text-xs font-semibold uppercase tracking-wider text-slate-500">
        性能统计
      </h3>

      <div className="grid grid-cols-2 gap-3">
        {/* FPS */}
        <div className="rounded-md bg-slate-800/50 px-3 py-2">
          <p className="text-[10px] text-slate-500">FPS</p>
          <p className={`text-lg font-bold font-mono ${fpsColor}`}>{fps}</p>
        </div>

        {/* Packets */}
        <div className="rounded-md bg-slate-800/50 px-3 py-2">
          <p className="text-[10px] text-slate-500">数据包</p>
          <p className="text-lg font-bold font-mono text-slate-200">
            {packetCount.toLocaleString()}
          </p>
        </div>

        {/* Uptime */}
        <div className="rounded-md bg-slate-800/50 px-3 py-2">
          <p className="text-[10px] text-slate-500">连接时长</p>
          <p className="text-lg font-bold font-mono text-slate-200">
            {formatUptime(uptime)}
          </p>
        </div>

        {/* Status */}
        <div className="rounded-md bg-slate-800/50 px-3 py-2">
          <p className="text-[10px] text-slate-500">状态</p>
          <p className={`text-lg font-bold ${
            isStreaming
              ? 'text-emerald-400'
              : connectionStatus === 'connected'
                ? 'text-yellow-400'
                : 'text-red-400'
          }`}>
            {isStreaming ? '●' : connectionStatus === 'connected' ? '◐' : '○'}
          </p>
        </div>
      </div>

      {/* Connection Status Detail */}
      <div className="mt-3 flex items-center justify-between rounded bg-slate-800/30 px-3 py-1.5">
        <span className="text-[10px] text-slate-500">WebSocket</span>
        <span className={`text-[10px] font-medium ${
          connectionStatus === 'connected'
            ? 'text-emerald-400'
            : connectionStatus === 'connecting'
              ? 'text-yellow-400'
              : 'text-red-400'
        }`}>
          {connectionStatus.toUpperCase()}
        </span>
      </div>
    </div>
  );
}
