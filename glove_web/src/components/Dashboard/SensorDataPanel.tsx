import { useSensorStore } from '../../stores/useSensorStore';
import { FINGER_NAMES } from '../../utils/constants';

// ── Color based on sensor value range ──
function valueToColor(value: number): string {
  const abs = Math.abs(value);
  if (abs < 0.3) return 'bg-emerald-500';
  if (abs < 0.7) return 'bg-yellow-500';
  return 'bg-red-500';
}

function valueToTextColor(value: number): string {
  const abs = Math.abs(value);
  if (abs < 0.3) return 'text-emerald-400';
  if (abs < 0.7) return 'text-yellow-400';
  return 'text-red-400';
}

// ── Mini bar indicator ──
function SensorBar({ value, label }: {
  value: number;
  label: string;
}) {
  const absVal = Math.abs(value);
  const barWidth = Math.min(100, absVal * 100);
  const barColor = valueToColor(value);
  const textColor = valueToTextColor(value);

  return (
    <div className="group flex items-center gap-2 py-0.5">
      <span className="w-20 shrink-0 truncate text-[10px] text-slate-500" title={label}>
        {label}
      </span>
      <div className="h-1.5 flex-1 overflow-hidden rounded-full bg-slate-800">
        <div
          className={`h-full rounded-full transition-all duration-150 ${barColor}`}
          style={{ width: `${barWidth}%` }}
        />
      </div>
      <span className={`w-10 shrink-0 text-right font-mono text-[10px] ${textColor}`}>
        {value.toFixed(2)}
      </span>
    </div>
  );
}

export default function SensorDataPanel() {
  const hall = useSensorStore((s) => s.hall);
  const imu = useSensorStore((s) => s.imu);

  // Hall sensor labels
  const hallLabels = [
    'Thumb CMC', 'Thumb MCP', 'Thumb IP',
    'Index MCP', 'Index PIP', 'Index DIP',
    'Middle MCP', 'Middle PIP', 'Middle DIP',
    'Ring MCP', 'Ring PIP', 'Ring DIP',
    'Pinky MCP', 'Pinky PIP', 'Pinky DIP',
  ];

  // IMU labels
  const imuLabels = [
    'Gyro X', 'Gyro Y', 'Gyro Z',
    'Accel X', 'Accel Y', 'Accel Z',
  ];

  return (
    <div className="animate-fade-in rounded-lg border border-slate-800 bg-slate-900/50 p-4">
      {/* Hall Sensors */}
      <div className="mb-4">
        <div className="mb-2 flex items-center justify-between">
          <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-500">
            霍尔传感器 (15)
          </h3>
          <span className="text-[10px] text-slate-600">Hall Effect</span>
        </div>
        <div className="space-y-0.5">
          {hall.map((value, i) => (
            <SensorBar
              key={`hall-${i}`}
              value={value}
              label={hallLabels[i] ?? FINGER_NAMES[i + 1] ?? `H${i}`}
            />
          ))}
        </div>
      </div>

      {/* IMU Sensors */}
      <div>
        <div className="mb-2 flex items-center justify-between">
          <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-500">
            IMU 传感器 (6)
          </h3>
          <span className="text-[10px] text-slate-600">Gyro + Accel</span>
        </div>
        <div className="space-y-0.5">
          {imu.map((value, i) => (
            <SensorBar
              key={`imu-${i}`}
              value={value}
              label={imuLabels[i] ?? `IMU_${i}`}
            />
          ))}
        </div>
      </div>
    </div>
  );
}
