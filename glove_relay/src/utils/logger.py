# -*- coding: utf-8 -*-
"""
glove_relay.src.utils.logger — Logging setup with coloured console output.

Usage
-----
    from src.utils.logger import get_logger
    log = get_logger(__name__)
    log.info("Hello from %s", __name__)
"""

from __future__ import annotations

import logging
import sys
from typing import Optional


class _ColourFormatter(logging.Formatter):
    """ANSI colour formatter for console log output."""

    # ANSI colour codes
    _COLOURS = {
        logging.DEBUG: "\033[36m",       # cyan
        logging.INFO: "\033[32m",         # green
        logging.WARNING: "\033[33m",      # yellow
        logging.ERROR: "\033[31m",        # red
        logging.CRITICAL: "\033[1;31m",   # bold red
    }
    _RESET = "\033[0m"

    def format(self, record: logging.LogRecord) -> str:
        colour = self._COLOURS.get(record.levelno, self._RESET)
        record.levelname = f"{colour}{record.levelname:<8}{self._RESET}"
        return super().format(record)


# ---------------------------------------------------------------------------
# Package-level logger configuration
# ---------------------------------------------------------------------------
_root_handler: Optional[logging.StreamHandler] = None
_configured: bool = False


def _ensure_configured() -> None:
    """Configure the root logger exactly once."""
    global _root_handler, _configured  # noqa: PLW0603
    if _configured:
        return

    # Default format before config is loaded
    fmt = "%(asctime)s | %(levelname)-8s | %(name)s | %(message)s"
    formatter = _ColourFormatter(fmt, datefmt="%H:%M:%S")

    _root_handler = logging.StreamHandler(sys.stdout)
    _root_handler.setFormatter(formatter)
    _root_handler.setLevel(logging.DEBUG)

    root = logging.getLogger("glove_relay")
    root.addHandler(_root_handler)
    root.setLevel(logging.DEBUG)
    root.propagate = False

    _configured = True


def get_logger(name: str) -> logging.Logger:
    """
    Return a named logger under the ``glove_relay`` namespace.

    Parameters
    ----------
    name:
        Typically ``__name__`` of the calling module.  The function strips
        the leading ``src.`` prefix so that log messages show concise module
        names like ``main`` instead of ``src.main``.

    Returns
    -------
    logging.Logger
    """
    _ensure_configured()

    # Normalise: "src.main" → "main", "glove_relay.src.utils" → "src.utils"
    if name.startswith("glove_relay."):
        name = name[len("glove_relay."):]
    return logging.getLogger(f"glove_relay.{name}")
