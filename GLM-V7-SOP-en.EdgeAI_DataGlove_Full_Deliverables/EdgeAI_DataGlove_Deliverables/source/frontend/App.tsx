import React, { useState, useEffect } from 'react';
import { Activity, Cpu, Wifi, Layers, Hand, MessageSquare, Speaker } from 'lucide-react';
import { motion, AnimatePresence } from 'motion/react';

const StatCard = ({ icon: Icon, title, value, subValue, color }: any) => (
  <div className="bg-white p-6 rounded-2xl shadow-sm border border-slate-100 flex flex-col gap-4">
    <div className="flex items-center justify-between">
      <div className={`p-2 rounded-lg ${color}`}>
        <Icon className="w-5 h-5 text-white" />
      </div>
      <span className="text-xs font-medium text-slate-400 uppercase tracking-wider">{title}</span>
    </div>
    <div>
      <div className="text-2xl font-bold text-slate-800">{value}</div>
      <div className="text-sm text-slate-500">{subValue}</div>
    </div>
  </div>
);

export default function App() {
  const [status, setStatus] = useState('Connected');
  const [l1Gesture, setL1Gesture] = useState('REST');
  const [l2Gesture, setL2Gesture] = useState('NONE');
  const [latency, setLatency] = useState(40.3);
  const [sentence, setSentence] = useState('I eat apple');

  return (
    <div className="min-h-screen bg-slate-50 p-8 font-sans">
      <header className="max-w-7xl mx-auto mb-12 flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold text-slate-900 tracking-tight">Edge-AI Data Glove</h1>
          <p className="text-slate-500 mt-1">Dual-Tier Inference Dashboard</p>
        </div>
        <div className="flex items-center gap-3 bg-emerald-50 text-emerald-700 px-4 py-2 rounded-full border border-emerald-100">
          <div className="w-2 h-2 bg-emerald-500 rounded-full animate-pulse" />
          <span className="text-sm font-medium">System Online</span>
        </div>
      </header>

      <main className="max-w-7xl mx-auto grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-12">
        <StatCard 
          icon={Wifi} 
          title="Connectivity" 
          value="UDP @ 100Hz" 
          subValue="192.168.1.100:8888" 
          color="bg-blue-500" 
        />
        <StatCard 
          icon={Cpu} 
          title="L1 Edge (ESP32)" 
          value={l1Gesture} 
          subValue="Latency: 2.8ms" 
          color="bg-indigo-500" 
        />
        <StatCard 
          icon={Layers} 
          title="L2 Upper (PC)" 
          value={l2Gesture} 
          subValue="ST-GCN Escalation" 
          color="bg-purple-500" 
        />
        <StatCard 
          icon={Activity} 
          title="Total Latency" 
          value={`${latency}ms`} 
          subValue="End-to-End" 
          color="bg-rose-500" 
        />
      </main>

      <div className="max-w-7xl mx-auto grid grid-cols-1 lg:grid-cols-3 gap-8">
        {/* Hand Visualization Placeholder */}
        <div className="lg:col-span-2 bg-white rounded-3xl shadow-sm border border-slate-100 overflow-hidden min-h-[400px] flex flex-col">
          <div className="p-6 border-b border-slate-50 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <Hand className="w-5 h-5 text-slate-400" />
              <span className="font-semibold text-slate-700">ms-MANO 3D Rendering</span>
            </div>
            <span className="text-xs text-slate-400">Unity Engine @ 60FPS</span>
          </div>
          <div className="flex-1 bg-slate-900 flex items-center justify-center relative">
            <div className="text-slate-500 text-sm italic">3D Hand Model Visualization Area</div>
            {/* Simulated Hand Motion Accents */}
            <motion.div 
              animate={{ scale: [1, 1.05, 1] }}
              transition={{ duration: 2, repeat: Infinity }}
              className="absolute inset-0 bg-gradient-to-br from-indigo-500/10 to-transparent pointer-events-none" 
            />
          </div>
        </div>

        {/* NLP & Translation Output */}
        <div className="flex flex-col gap-6">
          <div className="bg-white p-8 rounded-3xl shadow-sm border border-slate-100 flex-1">
            <div className="flex items-center gap-2 mb-6">
              <MessageSquare className="w-5 h-5 text-slate-400" />
              <span className="font-semibold text-slate-700">NLP Translation</span>
            </div>
            
            <div className="space-y-6">
              <div>
                <label className="text-xs font-bold text-slate-400 uppercase">Raw Sequence (SOV)</label>
                <div className="mt-2 p-4 bg-slate-50 rounded-xl font-mono text-sm text-slate-600">
                  I APPLE EAT
                </div>
              </div>
              
              <div>
                <label className="text-xs font-bold text-slate-400 uppercase">Corrected (SVO)</label>
                <motion.div 
                  initial={{ opacity: 0, y: 10 }}
                  animate={{ opacity: 1, y: 0 }}
                  className="mt-2 p-6 bg-indigo-50 rounded-2xl text-xl font-bold text-indigo-700 border border-indigo-100"
                >
                  {sentence}
                </motion.div>
              </div>

              <button className="w-full py-4 bg-slate-900 text-white rounded-2xl font-semibold flex items-center justify-center gap-2 hover:bg-slate-800 transition-colors">
                <Speaker className="w-5 h-5" />
                Play TTS Output
              </button>
            </div>
          </div>

          <div className="bg-slate-900 p-6 rounded-3xl text-white">
            <div className="flex items-center justify-between mb-4">
              <span className="text-sm font-medium text-slate-400">System Logs</span>
              <span className="text-[10px] bg-slate-800 px-2 py-1 rounded uppercase">Live</span>
            </div>
            <div className="space-y-2 font-mono text-[11px] text-slate-400">
              <p><span className="text-emerald-400">[OK]</span> WiFi Connected: 192.168.1.100</p>
              <p><span className="text-emerald-400">[OK]</span> BNO085 Initialized (0x4A)</p>
              <p><span className="text-emerald-400">[OK]</span> 5x TMAG5273 Detected via TCA9548A</p>
              <p><span className="text-blue-400">[INFO]</span> L1 Model Loaded (38KB INT8)</p>
              <p><span className="text-amber-400">[WARN]</span> L1 Confidence 0.72 &lt; 0.85, Escalating to L2...</p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
