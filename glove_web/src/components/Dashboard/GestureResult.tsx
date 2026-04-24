import { useGestureStore, selectL1Label } from '../../stores/useGestureStore';
import { GESTURE_LABELS } from '../../utils/constants';

// ── Confidence Bar Component ──
function ConfidenceBar({ label, confidence, color }: {
  label: string;
  confidence: number | null;
  color: string;
}) {
  const pct = confidence !== null ? Math.round(confidence * 100) : 0;

  return (
    <div className="space-y-1">
      <div className="flex items-center justify-between text-xs">
        <span className="text-slate-400">{label}</span>
        <span className="font-mono text-slate-300">{pct}%</span>
      </div>
      <div className="h-2 w-full overflow-hidden rounded-full bg-slate-800">
        <div
          className={`h-full rounded-full transition-all duration-200 ${color}`}
          style={{ width: `${pct}%` }}
        />
      </div>
    </div>
  );
}

export default function GestureResult() {
  const l1Label = useGestureStore(selectL1Label);
  const l1Confidence = useGestureStore((s) => s.l1Confidence);
  const l2Confidence = useGestureStore((s) => s.l2Confidence);
  const nlpText = useGestureStore((s) => s.nlpText);
  const history = useGestureStore((s) => s.gestureHistory);

  return (
    <div className="animate-fade-in rounded-lg border border-slate-800 bg-slate-900/50 p-4">
      {/* Current Gesture */}
      <div className="mb-4">
        <h3 className="mb-2 text-xs font-semibold uppercase tracking-wider text-slate-500">
          当前识别结果
        </h3>

        {/* Primary Gesture */}
        <div className="mb-3 flex items-center gap-3">
          <div className="flex h-14 w-14 items-center justify-center rounded-lg bg-blue-500/20 text-2xl">
            ✋
          </div>
          <div className="flex-1">
            <p className="text-lg font-bold text-slate-100">{l1Label}</p>
            <p className="text-xs text-slate-400">
              L1 Gesture #{useGestureStore.getState().l1GestureId ?? '—'}
            </p>
          </div>
        </div>

        {/* Confidence Bars */}
        <div className="space-y-2">
          <ConfidenceBar
            label="L1 置信度"
            confidence={l1Confidence}
            color="bg-blue-500"
          />
          <ConfidenceBar
            label="L2 置信度"
            confidence={l2Confidence}
            color="bg-emerald-500"
          />
        </div>
      </div>

      {/* NLP Text */}
      {nlpText && (
        <div className="mb-4 rounded-md border border-slate-700 bg-slate-800/50 p-3">
          <p className="mb-1 text-xs text-slate-500">NLP 纠正文本</p>
          <p className="text-sm font-medium text-slate-200">{nlpText}</p>
        </div>
      )}

      {/* Gesture History */}
      <div>
        <div className="mb-2 flex items-center justify-between">
          <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-500">
            手势历史
          </h3>
          <span className="text-xs text-slate-600">{history.length} / 10</span>
        </div>
        {history.length === 0 ? (
          <p className="py-4 text-center text-xs text-slate-600">暂无历史记录</p>
        ) : (
          <ul className="max-h-48 space-y-1 overflow-y-auto">
            {history.map((entry, i) => (
              <li
                key={`${entry.timestamp}-${i}`}
                className="flex items-center justify-between rounded px-2 py-1.5 text-xs transition-colors hover:bg-slate-800"
              >
                <span className="font-medium text-slate-300">
                  {GESTURE_LABELS[entry.gestureId ?? 0] ?? entry.label}
                </span>
                <span className="font-mono text-slate-500">
                  {Math.round(entry.confidence * 100)}%
                </span>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  );
}
