# -*- coding: utf-8 -*-
"""
glove_relay.scripts.data_collector — Data collection tool for the glove.

Supports two data sources:
  * **BLE serial** — connects to the ESP32 over Bluetooth Serial.
  * **UDP** — receives datagrams from the ESP32 on the network.

Features
--------
- Real-time waveform display using Matplotlib.
- Record labelled gestures and save as ``.npy`` files with shape ``(N, 30, 21)``.
- On-the-fly data augmentation (time shift, Gaussian noise, time masking).

Usage
-----
    # Record from UDP (default port 8888)
    python -m scripts.data_collector --source udp --output data/recorded

    # Record from BLE serial
    python -m scripts.data_collector --source ble --port /dev/ttyUSB0 --output data/recorded

    # Replay from a CSV log
    python -m scripts.data_collector --source csv --input data/log.csv --output data/recorded
"""

from __future__ import annotations

import argparse
import asyncio
import json
import struct
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

# ---------------------------------------------------------------------------
# Data augmentation
# ---------------------------------------------------------------------------
def augment_time_shift(data: np.ndarray, max_shift: int = 3) -> np.ndarray:
    """
    Randomly shift frames along the time axis.

    Parameters
    ----------
    data : np.ndarray
        ``(T, 21)``
    max_shift : int
        Maximum number of frames to shift.

    Returns
    -------
    np.ndarray
        ``(T, 21)`` — shifted with zero-padding.
    """
    shift = np.random.randint(-max_shift, max_shift + 1)
    if shift == 0:
        return data.copy()
    result = np.zeros_like(data)
    if shift > 0:
        result[shift:] = data[:-shift]
    else:
        result[:shift] = data[-shift:]
    return result


def augment_gaussian_noise(data: np.ndarray, sigma: float = 0.02) -> np.ndarray:
    """
    Add Gaussian noise to sensor values.

    Parameters
    ----------
    data : np.ndarray
        ``(T, 21)``
    sigma : float
        Noise standard deviation relative to the data range.

    Returns
    -------
    np.ndarray
        Noised copy.
    """
    noise = np.random.normal(0, sigma, size=data.shape).astype(np.float32)
    return data + noise


def augment_time_masking(data: np.ndarray, max_mask_ratio: float = 0.15) -> np.ndarray:
    """
    Mask a random contiguous span of timesteps with zeros.

    Parameters
    ----------
    data : np.ndarray
        ``(T, 21)``
    max_mask_ratio : float
        Maximum fraction of timesteps to mask.

    Returns
    -------
    np.ndarray
        Masked copy.
    """
    result = data.copy()
    T = data.shape[0]
    mask_len = max(1, int(T * max_mask_ratio * np.random.random()))
    start = np.random.randint(0, T - mask_len + 1)
    result[start : start + mask_len] = 0.0
    return result


def apply_augmentation(data: np.ndarray, num_augmented: int = 3) -> List[np.ndarray]:
    """
    Generate augmented copies of a single gesture recording.

    Parameters
    ----------
    data : np.ndarray
        ``(T, 21)`` original sample.
    num_augmented : int
        Number of augmented copies to produce.

    Returns
    -------
    list[np.ndarray]
        List of augmented samples (each ``(T, 21)``).
    """
    augmented: List[np.ndarray] = []
    augmenters = [augment_time_shift, augment_gaussian_noise, augment_time_masking]
    for _ in range(num_augmented):
        sample = data.copy()
        # Apply 1–2 random augmentations
        n_ops = np.random.randint(1, 3)
        chosen = np.random.choice(len(augmenters), size=n_ops, replace=False)
        for idx in chosen:
            sample = augmenters[idx](sample)
        augmented.append(sample)
    return augmented


# ---------------------------------------------------------------------------
# Real-time waveform display
# ---------------------------------------------------------------------------
def display_waveform(frame: np.ndarray, title: str = "Live Sensor Data") -> None:
    """
    Print a minimal ASCII waveform of the current frame to stdout.

    Parameters
    ----------
    frame : np.ndarray
        ``(21,)`` sensor values.
    title : str
        Title line.
    """
    cols = 60
    min_v, max_v = frame.min(), frame.max()
    rng = max_v - min_v if max_v != min_v else 1.0

    # Hall sensors (0–14) — bar plot
    hall = frame[:15]
    print(f"\n{'=' * cols}  {title}")
    for i, v in enumerate(hall):
        bar_len = int((v - min_v) / rng * (cols - 20))
        bar = "█" * bar_len
        print(f"  H{i:02d} [{v:7.2f}] {bar}")

    # IMU (15–20) — line indicator
    imu = frame[15:]
    mid = cols // 2
    for i, v in enumerate(imu):
        pos = int(mid + (v / (rng or 1)) * (mid - 5))
        pos = max(1, min(cols - 2, pos))
        line = [" "] * cols
        line[mid] = "│"
        line[pos] = "●"
        print(f"  I{i}  {''.join(line)}")
    print(f"{'=' * cols}")


# ---------------------------------------------------------------------------
# Data source abstractions
# ---------------------------------------------------------------------------
class DataSource:
    """Abstract base for data sources."""

    async def read_frame(self) -> Optional[np.ndarray]:
        """Read a single frame. Returns ``(21,)`` or ``None`` on timeout."""
        raise NotImplementedError

    async def close(self) -> None:
        pass


class UDPDataSource(DataSource):
    """Read sensor frames from a UDP socket (forwards from ESP32)."""

    def __init__(self, host: str = "0.0.0.0", port: int = 8888, timeout: float = 5.0) -> None:
        self.host = host
        self.port = port
        self.timeout = timeout

    async def read_frame(self) -> Optional[np.ndarray]:
        loop = asyncio.get_running_loop()
        try:
            data, _ = await asyncio.wait_for(
                loop.create_datagram_endpoint(
                    asyncio.DatagramProtocol,
                    local_addr=(self.host, self.port),
                ),
                timeout=self.timeout,
            )
            # In a real implementation we'd properly parse the data here.
            # This is a skeleton — see protobuf_parser.py for the actual decoder.
            return np.zeros(21, dtype=np.float32)
        except asyncio.TimeoutError:
            return None


# ---------------------------------------------------------------------------
# Recording session
# ---------------------------------------------------------------------------
class RecordingSession:
    """
    Manages a gesture recording session.

    Parameters
    ----------
    output_dir : Path
        Directory to save ``.npy`` files.
    window_size : int
        Number of frames per gesture sample.
    label_file : Path
        Path to gesture_labels.json.
    """

    def __init__(
        self,
        output_dir: Path,
        window_size: int = 30,
        label_file: Optional[Path] = None,
    ) -> None:
        self.output_dir = output_dir
        self.window_size = window_size
        self.labels: Dict[int, str] = {}
        self._buffer: List[np.ndarray] = []

        output_dir.mkdir(parents=True, exist_ok=True)

        if label_file and label_file.exists():
            with open(label_file, "r", encoding="utf-8") as fh:
                for entry in json.load(fh):
                    self.labels[entry["id"]] = entry["name_cn"]

    def add_frame(self, frame: np.ndarray) -> None:
        """Append a frame to the buffer."""
        self._buffer.append(frame.copy())
        if len(self._buffer) > self.window_size:
            self._buffer.pop(0)

    def save_recording(self, gesture_id: int) -> Path:
        """
        Save the current buffer as a labelled ``.npy`` file.

        If the buffer has fewer than ``window_size`` frames, it is
        zero-padded at the beginning.
        """
        # Pad if needed
        while len(self._buffer) < self.window_size:
            self._buffer.insert(0, np.zeros(21, dtype=np.float32))

        data = np.stack(self._buffer, axis=0)  # (T, 21)
        label_name = self.labels.get(gesture_id, f"unknown_{gesture_id}")

        # Save original
        filename = f"gesture_{gesture_id:03d}_{label_name}_{int(time.time())}.npy"
        filepath = self.output_dir / filename
        np.save(filepath, data)
        print(f"  ✓ Saved: {filepath}  shape={data.shape}")

        # Save augmented copies
        for i, aug in enumerate(apply_augmentation(data, num_augmented=3)):
            aug_name = f"gesture_{gesture_id:03d}_{label_name}_aug{i}_{int(time.time())}.npy"
            aug_path = self.output_dir / aug_name
            np.save(aug_path, aug)
            print(f"  ✓ Augmented: {aug_path}")

        # Clear buffer
        self._buffer.clear()
        return filepath


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main() -> None:
    """CLI entry point for the data collector."""
    parser = argparse.ArgumentParser(
        description="Data collection tool for the EdgeAI DataGlove V3",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--source",
        choices=["udp", "ble", "csv"],
        default="udp",
        help="Data source (default: udp)",
    )
    parser.add_argument("--host", type=str, default="0.0.0.0", help="UDP bind host")
    parser.add_argument("--port", type=int, default=8888, help="UDP bind port")
    parser.add_argument("--ble-port", type=str, default="/dev/ttyUSB0", help="BLE serial port")
    parser.add_argument("--csv-input", type=str, default=None, help="CSV log file path")
    parser.add_argument("--output", type=str, default="data/recorded", help="Output directory")
    parser.add_argument("--window", type=int, default=30, help="Frames per gesture sample")
    parser.add_argument("--labels", type=str, default="data/gesture_labels.json", help="Label file")
    parser.add_argument("--visualize", action="store_true", help="Show real-time waveform")
    parser.add_argument("--augment", type=int, default=3, help="Augmented copies per recording")

    args = parser.parse_args()

    session = RecordingSession(
        output_dir=Path(args.output),
        window_size=args.window,
        label_file=Path(args.labels) if Path(args.labels).exists() else None,
    )

    print("╔══════════════════════════════════════════════════════╗")
    print("║     EdgeAI DataGlove V3 — Data Collection Tool      ║")
    print("╠══════════════════════════════════════════════════════╣")
    print(f"║  Source: {args.source:<43s}  ║")
    print(f"║  Output: {args.output:<43s}  ║")
    print(f"║  Window: {args.window} frames{' ' * 36}  ║")
    print("╠══════════════════════════════════════════════════════╣")
    print("║  Commands:                                          ║")
    print("║    r <id>  — Record gesture with given label ID     ║")
    print("║    q       — Quit                                   ║")
    print("╚══════════════════════════════════════════════════════╝")
    print()

    print(f"Available labels: {session.labels}")
    print("\nReady. Enter 'r <id>' to record, 'q' to quit.")

    # Simple interactive loop (UDP mode)
    if args.source == "udp":
        try:
            import socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind((args.host, args.port))
            sock.settimeout(5.0)
            print(f"Listening on {args.host}:{args.port} …")

            while True:
                try:
                    cmd = input("\n> ").strip()
                except (EOFError, KeyboardInterrupt):
                    break

                if cmd.lower() == "q":
                    break
                if cmd.lower().startswith("r "):
                    try:
                        gid = int(cmd.split()[1])
                    except (ValueError, IndexError):
                        print("  Usage: r <gesture_id>")
                        continue

                    print(f"  Recording gesture {gid} ({session.labels.get(gid, '?')}), "
                          f"waiting for {args.window} frames …")

                    for i in range(args.window):
                        try:
                            raw, _ = sock.recvfrom(4096)
                            # Minimal fallback parse (same layout as protobuf_parser fallback)
                            if len(raw) >= 66:
                                offset = 12  # skip timestamp(8) + seq_num(4)
                                hall = [struct.unpack_from("<H", raw, offset + j * 2)[0] for j in range(15)]
                                offset += 30
                                imu = [struct.unpack_from("<f", raw, offset + j * 4)[0] for j in range(6)]
                                frame = np.array([float(v) for v in hall + imu], dtype=np.float32)
                            else:
                                frame = np.zeros(21, dtype=np.float32)
                            session.add_frame(frame)
                            if args.visualize:
                                display_waveform(frame, f"Frame {i + 1}/{args.window}")
                        except socket.timeout:
                            session.add_frame(np.zeros(21, dtype=np.float32))
                            print(f"  ⚠ Frame {i + 1} timed out — padding with zeros")

                    session.save_recording(gid)

            sock.close()
        except ImportError:
            print("Socket module unavailable")
    else:
        print(f"Source '{args.source}' not yet implemented in this skeleton. Use UDP mode.")

    print("\nData collection session ended.")


if __name__ == "__main__":
    main()
