/* =============================================================================
 * EdgeAI Data Glove V3 — Kalman Filter 1D (Header-Only)
 * =============================================================================
 * Lightweight single-state Kalman filter for real-time sensor smoothing.
 * Applied to all 21 signals (15 Hall + 6 IMU) before feature extraction.
 *
 * Template Parameters:
 *   T — floating-point type (float for embedded, double if needed)
 *
 * V3 Note:
 *   In V2, KalmanFilter was applied post-hoc and could diverge on first
 *   boot due to uninitialized state. V3 auto-initializes the filter on
 *   the very first update() call by using the measurement as the initial
 *   estimate, preventing startup transients.
 *
 * Usage:
 *   KalmanFilter1D<float> kf(0.001f, 0.01f);  // Q=process, R=measurement noise
 *   float filtered = kf.update(raw_value);
 * =============================================================================
 */

#ifndef KALMAN_FILTER_1D_H
#define KALMAN_FILTER_1D_H

#include <cmath>
#include <algorithm>

template <typename T = float>
class KalmanFilter1D {
public:
    // =========================================================================
    // Construction
    // =========================================================================

    /**
     * @brief Construct a 1D Kalman filter.
     * @param process_noise   Q — estimate of measurement process noise variance.
     *                        Larger Q → filter tracks faster, less smoothing.
     *                        Typical: 0.0001 – 0.01
     * @param measurement_noise R — estimate of sensor measurement noise variance.
     *                          Larger R → filter trusts sensor less, more smoothing.
     *                          Typical: 0.001 – 0.1
     */
    explicit KalmanFilter1D(T process_noise = T(0.001), T measurement_noise = T(0.01))
        : Q(process_noise), R(measurement_noise),
          x(T(0)),       // State estimate
          P(T(1)),       // Estimate error covariance
          K(T(0)),       // Kalman gain
          _initialized(false) {}

    // =========================================================================
    // Core Filter Operation
    // =========================================================================

    /**
     * @brief Run one Kalman update cycle.
     *
     * Prediction step (for 1D constant model):
     *   x̂⁻ = x̂  (no velocity term in 1D)
     *   P⁻  = P + Q
     *
     * Update step:
     *   K   = P⁻ / (P⁻ + R)
     *   x̂   = x̂⁻ + K × (z - x̂⁻)
     *   P   = (1 - K) × P⁻
     *
     * @param measurement  Raw sensor reading.
     * @return Filtered estimate.
     */
    T update(T measurement) {
        if (!_initialized) {
            // First call: seed the filter with the actual measurement
            // to avoid large initial transient / divergence
            x = measurement;
            P = R;  // Start with measurement noise as initial uncertainty
            _initialized = true;
            return x;
        }

        // ---- Prediction ----
        // State does not change (constant model: x_next = x)
        T P_pred = P + Q;

        // ---- Update ----
        K = P_pred / (P_pred + R);
        x = x + K * (measurement - x);
        P = (T(1) - K) * P_pred;

        return x;
    }

    // =========================================================================
    // Accessors
    // =========================================================================

    /** @return Current filtered state estimate. */
    T getEstimate() const { return x; }

    /** @return Current error covariance (uncertainty). */
    T getErrorCovariance() const { return P; }

    /** @return Current Kalman gain. */
    T getKalmanGain() const { return K; }

    /** @return true if the filter has been seeded with at least one measurement. */
    bool isInitialized() const { return _initialized; }

    // =========================================================================
    // Tuning
    // =========================================================================

    /** @brief Adjust process noise Q at runtime. */
    void setProcessNoise(T q) { Q = std::max(T(1e-8), q); }

    /** @brief Adjust measurement noise R at runtime. */
    void setMeasurementNoise(T r) { R = std::max(T(1e-8), r); }

    // =========================================================================
    // Reset
    // =========================================================================

    /**
     * @brief Reset filter to uninitialized state.
     * Next update() call will re-seed from the measurement.
     */
    void reset() {
        x = T(0);
        P = T(1);
        K = T(0);
        _initialized = false;
    }

    /**
     * @brief Reset filter with a known initial estimate.
     * @param initial_value  Starting state estimate.
     * @param initial_p      Starting error covariance (default = R).
     */
    void resetTo(T initial_value, T initial_p = T(0)) {
        x = initial_value;
        P = (initial_p > T(0)) ? initial_p : R;
        K = T(0);
        _initialized = true;
    }

private:
    T Q;                  ///< Process noise variance
    T R;                  ///< Measurement noise variance
    T x;                  ///< State estimate (filtered output)
    T P;                  ///< Estimate error covariance
    T K;                  ///< Kalman gain (stored for diagnostics)
    bool _initialized;    ///< First-measurement guard
};

#endif // KALMAN_FILTER_1D_H
