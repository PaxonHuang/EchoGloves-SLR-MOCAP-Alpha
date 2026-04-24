# -*- coding: utf-8 -*-
"""
glove_relay.src.ws_server — WebSocket connection manager.

Manages multiple concurrent WebSocket clients and provides a single
``broadcast()`` method that the UDP / inference pipeline can call to push
JSON sensor+inference data to every connected frontend.
"""

from __future__ import annotations

import asyncio
import json
from typing import Any

from fastapi import WebSocket

from src.utils.logger import get_logger

logger = get_logger(__name__)


class ConnectionManager:
    """Thread-safe (single event-loop) manager for active WebSocket connections."""

    def __init__(self) -> None:
        self._connections: list[WebSocket] = []

    # ------------------------------------------------------------------
    # Connection lifecycle
    # ------------------------------------------------------------------
    async def connect(self, websocket: WebSocket) -> None:
        """Accept the handshake and register *websocket*."""
        await websocket.accept()
        self._connections.append(websocket)
        logger.info("WebSocket client connected (total %d)", len(self._connections))

    def disconnect(self, websocket: WebSocket) -> None:
        """Remove *websocket* from the active set."""
        if websocket in self._connections:
            self._connections.remove(websocket)
            logger.info("WebSocket client disconnected (total %d)", len(self._connections))

    @property
    def active_count(self) -> int:
        """Number of currently connected clients."""
        return len(self._connections)

    # ------------------------------------------------------------------
    # Broadcast
    # ------------------------------------------------------------------
    async def broadcast(self, data: dict[str, Any]) -> None:
        """
        Send *data* as JSON to every connected WebSocket client.

        Payload schema
        --------------
        .. code-block:: json

            {
                "timestamp":   1718294400.123,
                "hall":        [1023, 512, ...],       // 15 flex-sensor values
                "imu":         [0.12, -0.34, ...],     // 6 IMU values
                "l1_gesture_id": 5,
                "l1_confidence": 0.97,
                "l2_gesture_id": -1,
                "l2_confidence": 0.0,
                "nlp_text":    "你好",
                "status":      "l1_ok"
            }
        """
        if not self._connections:
            return

        payload = json.dumps(data, ensure_ascii=False)
        dead: list[WebSocket] = []

        for ws in self._connections:
            try:
                await ws.send_text(payload)
            except Exception:
                logger.warning("Failed to send to a WebSocket client — marking for removal")
                dead.append(ws)

        for ws in dead:
            self.disconnect(ws)

    # ------------------------------------------------------------------
    # Cleanup
    # ------------------------------------------------------------------
    async def close_all(self) -> None:
        """Gracefully close every connection."""
        for ws in list(self._connections):
            try:
                await ws.close(code=1001, reason="server shutdown")
            except Exception:
                pass
        self._connections.clear()
        logger.info("All WebSocket connections closed")
