# -*- coding: utf-8 -*-
"""
glove_relay.src.protobuf_parser — Parse GloveData Protobuf messages.

The ESP32 firmware serialises sensor readings using ``glove_data.proto``.
This module converts raw bytes into a plain ``dict`` suitable for JSON
serialisation over WebSocket.

.. note::
    The compiled ``_pb2`` module is expected at ``proto/glove_data_pb2.py``.
    Generate it with::

        protoc --python_out=proto --proto_path=proto proto/glove_data.proto
"""

from __future__ import annotations

from typing import Any

from src.utils.logger import get_logger

logger = get_logger(__name__)

# ---------------------------------------------------------------------------
# Attempt to import the generated protobuf module.  If it is not yet compiled
# (e.g. during first-time setup) we fall back to a lightweight manual parser
# that can decode a subset of well-known fields.
# ---------------------------------------------------------------------------
try:
    from proto import glove_data_pb2  # type: ignore[import-untyped]

    _HAS_PROTOBUF = True
except ImportError:
    _HAS_PROTOBUF = False
    logger.warning(
        "proto/glove_data_pb2.py not found — using fallback parser.  "
        "Run `protoc --python_out=proto proto/glove_data.proto` for full support."
    )


def parse_glove_data(data: bytes) -> dict[str, Any]:
    """
    Parse a single GloveData protobuf message into a dictionary.

    Parameters
    ----------
    data:
        Raw protobuf-encoded bytes received via UDP.

    Returns
    -------
    dict
        A JSON-serialisable dictionary with the following top-level keys:

        * ``timestamp`` (*float*) — UTC epoch seconds from the ESP32.
        * ``hall`` (*list[float]*) — 15 hall-effect / flex sensor readings.
        * ``imu`` (*list[float]*) — 6 IMU readings (accel_x/y/z, gyro_x/y/z).
        * ``seq_num`` (*int*) — monotonically increasing sequence number.

    Raises
    ------
    ValueError
        If *data* cannot be decoded as a valid ``GloveData`` message.
    """
    if _HAS_PROTOBUF:
        return _parse_with_pb2(data)
    return _parse_fallback(data)


# ---------------------------------------------------------------------------
# Full protobuf decoder
# ---------------------------------------------------------------------------
def _parse_with_pb2(data: bytes) -> dict[str, Any]:
    """Decode using the compiled ``glove_data_pb2`` module."""
    msg = glove_data_pb2.GloveData()
    try:
        msg.ParseFromString(data)
    except Exception as exc:
        raise ValueError(f"Protobuf decode error: {exc}") from exc

    result: dict[str, Any] = {
        "timestamp": msg.timestamp,
        "seq_num": msg.seq_num,
        "hall": list(msg.hall_sensor),
        "imu": list(msg.imu_data),
    }

    # Optional quaternion field (if present in proto)
    if hasattr(msg, "quaternion") and msg.quaternion:
        result["quaternion"] = list(msg.quaternion)

    return result


# ---------------------------------------------------------------------------
# Minimal fallback parser (works with the V3 firmware wire format)
# ---------------------------------------------------------------------------
def _parse_fallback(data: bytes) -> dict[str, Any]:
    """
    Best-effort parser that can decode a flat binary payload.

    Expected layout (V3 firmware, 115 bytes):
        [timestamp: 8B float64]
        [seq_num:   4B uint32 ]
        [hall:     15B × 2B uint16 LE]
        [imu:       6B × 4B float32 LE]

    Total: 8 + 4 + 30 + 24 = 66 bytes.  Older firmware versions may have
    different layouts; this function is a safety net only.
    """
    import struct
    import time

    if len(data) < 66:
        raise ValueError(f"Fallback parser: expected ≥66 bytes, got {len(data)}")

    offset = 0

    timestamp = struct.unpack_from("<d", data, offset)[0]
    offset += 8

    seq_num = struct.unpack_from("<I", data, offset)[0]
    offset += 4

    hall_values: list[float] = []
    for _ in range(15):
        val = struct.unpack_from("<H", data, offset)[0]
        hall_values.append(float(val))
        offset += 2

    imu_values: list[float] = []
    for _ in range(6):
        val = struct.unpack_from("<f", data, offset)[0]
        imu_values.append(float(val))
        offset += 4

    # Sanity check — if timestamp is 0 use current time
    if timestamp == 0.0:
        timestamp = time.time()

    return {
        "timestamp": timestamp,
        "seq_num": seq_num,
        "hall": hall_values,
        "imu": imu_values,
    }
