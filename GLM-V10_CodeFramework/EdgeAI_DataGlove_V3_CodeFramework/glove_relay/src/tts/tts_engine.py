# -*- coding: utf-8 -*-
"""
glove_relay.src.tts.tts_engine — Edge-TTS integration for Mandarin speech synthesis.

Uses Microsoft Edge's online TTS service via the ``edge-tts`` library to
convert corrected Mandarin text into MP3 audio bytes.

Features
--------
- Asynchronous synthesis (non-blocking).
- Local file-system caching to avoid re-synthesising identical text.
- Configurable voice, rate, and volume.

Usage
-----
    from src.tts.tts_engine import TTSEngine

    engine = TTSEngine()
    audio_bytes = await engine.synthesize("你好，欢迎使用")
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
from typing import Optional

from src.utils.config import get_config
from src.utils.logger import get_logger

logger = get_logger(__name__)


class TTSEngine:
    """
    Async TTS engine backed by ``edge-tts``.

    Parameters
    ----------
    voice : str
        Microsoft Edge TTS voice name.  Defaults to ``zh-CN-XiaoxiaoNeural``.
    rate : str
        Speech rate adjustment (e.g. ``"+10%"``, ``"-5%"``).
    volume : str
        Volume adjustment (e.g. ``"+0%"``).
    cache_dir : str | Path
        Directory for caching synthesised audio files.
    """

    def __init__(
        self,
        voice: Optional[str] = None,
        rate: Optional[str] = None,
        volume: Optional[str] = None,
        cache_dir: Optional[str | Path] = None,
    ) -> None:
        config = get_config()
        self.voice: str = voice or config.tts.voice
        self.rate: str = rate or config.tts.rate
        self.volume: str = volume or config.tts.volume
        self.cache_dir: Path = Path(cache_dir or config.tts.cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self._cache: dict[str, Path] = {}

        logger.info(
            "TTS engine initialised — voice=%s, cache=%s",
            self.voice,
            self.cache_dir,
        )

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------
    async def synthesize(self, text: str, voice: Optional[str] = None) -> bytes:
        """
        Synthesize *text* to MP3 audio bytes.

        Parameters
        ----------
        text :
            Mandarin text to speak.  Empty or whitespace-only strings
            return an empty ``bytes`` result.
        voice :
            Override the default voice for this call.

        Returns
        -------
        bytes
            MP3-encoded audio data.

        Raises
        ------
        RuntimeError
            If the ``edge-tts`` library is not installed or synthesis fails.
        """
        text = text.strip()
        if not text:
            return b""

        # Check cache
        cache_key = self._cache_key(text, voice or self.voice)
        cached_path = self._cache.get(cache_key)
        if cached_path is not None and cached_path.exists():
            logger.debug("TTS cache hit: %s", cached_path.name)
            return cached_path.read_bytes()

        # Synthesize
        try:
            import edge_tts  # type: ignore[import-untyped]
        except ImportError as exc:
            raise RuntimeError(
                "edge-tts is not installed.  Run: pip install edge-tts"
            ) from exc

        communicate = edge_tts.Communicate(
            text=text,
            voice=voice or self.voice,
            rate=self.rate,
            volume=self.volume,
        )

        # Collect audio chunks
        audio_data = bytearray()
        async for chunk in communicate.stream():
            if chunk["type"] == "audio":
                audio_data.extend(chunk["data"])

        result = bytes(audio_data)

        # Write to cache
        cache_path = self.cache_dir / f"{cache_key}.mp3"
        try:
            cache_path.write_bytes(result)
            self._cache[cache_key] = cache_path
            logger.debug("TTS cached: %s (%d bytes)", cache_path.name, len(result))
        except OSError:
            logger.warning("Failed to write TTS cache: %s", cache_path)

        return result

    async def synthesize_to_file(self, text: str, output_path: str | Path) -> Path:
        """
        Synthesize *text* and write the result to *output_path*.

        Returns the path to the generated file.
        """
        audio = await self.synthesize(text)
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_bytes(audio)
        logger.info("TTS file written: %s (%d bytes)", output_path, len(audio))
        return output_path

    # ------------------------------------------------------------------
    # Cache management
    # ------------------------------------------------------------------
    def clear_cache(self) -> int:
        """Remove all cached files and return the count of deleted files."""
        count = 0
        for path in self.cache_dir.glob("*.mp3"):
            try:
                path.unlink()
                count += 1
            except OSError:
                pass
        self._cache.clear()
        logger.info("TTS cache cleared (%d files removed)", count)
        return count

    @staticmethod
    def _cache_key(text: str, voice: str) -> str:
        """Generate a deterministic cache key from text + voice."""
        raw = f"{voice}:{text}"
        return hashlib.sha256(raw.encode("utf-8")).hexdigest()[:16]
