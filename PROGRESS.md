# PROGRESS.md — Cross-Session State Tracker

**Last updated**: 2026-05-06

---

## MCP Plugin Status (Verified 2026-05-06)

| Plugin | Status | Notes |
|--------|--------|-------|
| Playwright | WORKING | `browser_tabs` list returns correctly |
| Chrome DevTools | WORKING | `list_pages` returns correctly |
| Context7 | FAILING | `TypeError: fetch failed` — proxy at 127.0.0.1:15721 likely blocks direct API calls |
| GitHub MCP | FAILING | `Authentication Failed: Bad credentials` — token added to settings.local.json, needs session restart to take effect |
| Espressif Docs | FAILING | `Socket connection closed unexpectedly` — server instability, may also be proxy-related |

### GitHub MCP Fix Applied
- Added `GITHUB_PERSONAL_ACCESS_TOKEN` env var to `.claude/settings.local.json`
- Token scopes: `repo, public_repo` (sufficient for PRs, issues, code search)
- **Action needed**: Restart Claude Code session for token to take effect

### Context7 & Espressif Docs
- Both fail with network errors — the proxy at `127.0.0.1:15721` (ANTHROPIC_BASE_URL) likely prevents MCP servers from making outbound HTTP calls
- These are transient connection issues, not configuration problems
- May work intermittently or after proxy/network changes

---

## Session Continuation Protocol

When starting a new session:
1. Read this file first
2. Check the MCP status table — skip re-testing if verified recently
3. Continue from the last checkpoint below
4. Update this file when completing a task

---

## Completed Checkpoints

- [x] MCP plugin verification (2026-05-06) — Playwright + Chrome DevTools confirmed working, GitHub token configured, Context7/Espressif Docs have proxy issues
- [x] PROGRESS.md created (2026-05-06)
- [x] CLAUDE.md updated with MCP status section (2026-05-06)
- [ ] Verify GitHub MCP works after session restart (needs restart to pick up env var)

---

## Phase 1 + Phase 2 Completion (2026-05-07)

### Phase 1: HAL & Driver Layer — COMPLETE

| Component | File | Status |
|-----------|------|--------|
| TCA9548A I2C mux driver | `lib/Sensors/TCA9548A.h/.cpp` | Complete (disableAll→selectChannel two-step, 1ms bus delay) |
| TMAG5273 Hall sensor driver | `lib/Sensors/TMG5273.h/.cpp` | Complete (header-only, 32× avg, ±40mT, Set/Reset trigger) |
| BNO085 IMU integration | `lib/Sensors/SensorManager.h` | Complete (Game Rotation Vector + Calibrated Gyroscope @ 100Hz) |
| SensorManager unified HAL | `lib/Sensors/SensorManager.h` | Complete (I2C init, mux, Hall array, IMU, Kalman filtering, quat→Euler) |
| FlexManager placeholder | `lib/Sensors/FlexManager.h` | Complete (V3.0 returns zeros, V3.1 will use ADC) |
| FreeRTOS dual-core tasks | `src/main.cpp` | Complete (static_assert validation, correct parameter order) |

**Key fix**: BNO085 uses `SH2_GAME_ROTATION_VECTOR` (no geomagnetic) per SOP spec §4.2, not `SH2_ROTATION_VECTOR`.

### Phase 2: Signal Processing & Data Acquisition — COMPLETE

| Component | File | Status |
|-----------|------|--------|
| Kalman Filter 1D | `lib/Filters/KalmanFilter1D.h` | Complete (21 channels, auto-seed on first update) |
| Sliding Window Ring Buffer | `lib/Filters/SlidingWindow.h` | Complete (30×21 floats, PSRAM allocation, SPSC) |
| Feature Normalizer | `lib/Filters/FeatureNormalizer.h` | Complete (Min-Max [0,1], 2s calibration, per-channel stats) |
| Pipeline integration | `src/main.cpp` | Complete (readAll→toFeatureArray→normalize→push→queue→CSV) |
| Serial CSV output | `src/main.cpp` | Complete (Edge Impulse data forwarder compatible) |

**Build verified**: `pio run` exit code 0 (confirmed 2026-05-07)

### Signal Processing Pipeline Flow

```
SensorManager.readAll()     → SensorData (Kalman-filtered inside SensorManager)
SensorData.toFeatureArray() → float[21] features
FeatureNormalizer.updateStats() → during 2s calibration
FeatureNormalizer.normalize()  → features mapped to [0,1]
SlidingWindow.push()           → ring buffer (30 frames)
FreeRTOS queue send            → SensorData to g_data_queue
Serial CSV output              → Edge Impulse compatible format
```

### Unit Tests Created

| Test File | Coverage |
|-----------|----------|
| `tests/test_tca9548a.cpp` | TCA9548A channel selection, disableAll, probe |
| `tests/test_tmag5273.cpp` | TMAG5273 begin, readXYZ, null mux handling |
| `tests/test_euler_conversion.cpp` | quat→Euler (5 cases), SlidingWindow (5 cases), FeatureNormalizer (5 cases) |

---

## Active Work

**Next**: Phase 3 — L1 Edge Inference (TinyML / TFLite Micro)