/**
 * Kalman 1D Filter
 *
 * Single-state Kalman filter for noise reduction
 * - Constant velocity model (A = 1)
 * - Process noise Q: sensor inherent noise
 * - Measurement noise R: measurement uncertainty
 *
 * Tuning:
 * - Larger Q = faster tracking, weaker filtering
 * - Larger R = stronger filtering, slower response
 * - Recommended: Q = 0.0001, R = 0.01 for Hall sensors
 */

#ifndef KALMAN_1D_H
#define KALMAN_1D_H

class Kalman1D {
public:
    /**
     * Constructor
     * @param Q Process noise covariance (default 0.0001)
     * @param R Measurement noise covariance (default 0.01)
     */
    Kalman1D(float Q = 0.0001f, float R = 0.01f)
        : _Q(Q), _R(R), _error_cov(1.0f), _x(0.0f), _initialized(false) {}

    /**
     * Update filter with new measurement
     * @param measurement Current sensor reading
     * @return Filtered estimate
     */
    float update(float measurement) {
        if (!_initialized) {
            // Initialize with first measurement
            _x = measurement;
            _initialized = true;
            return _x;
        }

        // Predict step
        // State prediction: x = x (constant velocity model)
        // Error covariance prediction: P = P + Q
        _error_cov = _error_cov + _Q;

        // Update step
        // Kalman gain: K = P / (P + R)
        float K = _error_cov / (_error_cov + _R);

        // State update: x = x + K * (z - x)
        _x = _x + K * (measurement - _x);

        // Error covariance update: P = (1 - K) * P
        _error_cov = (1.0f - K) * _error_cov;

        return _x;
    }

    /**
     * Get current filtered estimate
     */
    float getState() const {
        return _x;
    }

    /**
     * Get current error covariance
     */
    float getErrorCovariance() const {
        return _error_cov;
    }

    /**
     * Set process and measurement noise parameters
     * @param Q New process noise
     * @param R New measurement noise
     */
    void setParameters(float Q, float R) {
        _Q = Q;
        _R = R;
    }

    /**
     * Reset filter state
     */
    void reset() {
        _x = 0.0f;
        _error_cov = 1.0f;
        _initialized = false;
    }

    /**
     * Reset with initial value
     * @param initialValue Initial state estimate
     */
    void reset(float initialValue) {
        _x = initialValue;
        _error_cov = 1.0f;
        _initialized = true;
    }

private:
    float _Q;  // Process noise covariance
    float _R;  // Measurement noise covariance
    float _error_cov;  // Error covariance estimate (renamed to avoid ctype.h _P macro conflict)
    float _x;  // State estimate (filtered value)
    bool _initialized;
};

#endif  // KALMAN_1D_H