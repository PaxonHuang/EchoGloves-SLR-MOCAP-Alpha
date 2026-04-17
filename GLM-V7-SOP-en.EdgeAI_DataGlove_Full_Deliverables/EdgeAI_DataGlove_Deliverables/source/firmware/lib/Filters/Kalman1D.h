#ifndef KALMAN_1D_H
#define KALMAN_1D_H

class Kalman1D {
public:
    Kalman1D(float Q = 0.0001f, float R = 0.01f, float P = 1.0f, float initial_value = 0.0f)
        : Q(Q), R(R), P(P), x(initial_value) {}

    float update(float measurement) {
        // Prediction step
        // x = x; (constant velocity model A=1)
        P = P + Q;

        // Update step
        float K = P / (P + R);
        x = x + K * (measurement - x);
        P = (1.0f - K) * P;

        return x;
    }

    void setParameters(float newQ, float newR) {
        Q = newQ;
        R = newR;
    }

    float getState() const { return x; }

private:
    float Q; // Process noise covariance
    float R; // Measurement noise covariance
    float P; // Error covariance
    float x; // State estimate
};

#endif
