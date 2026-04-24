// =============================================================================
// HandPoseData.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// Data structures shared across networking, hand control, and UI modules.
// =============================================================================

using System;
using UnityEngine;

namespace EdgeAI.DataGlove
{
    // ---------------------------------------------------------------------------
    // 3-D point used for each hand keypoint.
    // ---------------------------------------------------------------------------
    [Serializable]
    public class Keypoint3D
    {
        public float x;
        public float y;
        public float z;

        public Keypoint3D() { x = y = z = 0f; }

        public Keypoint3D(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        /// <summary>Implicit conversion to UnityEngine.Vector3 for convenience.</summary>
        public static implicit operator Vector3(Keypoint3D k)
            => new Vector3(k.x, k.y, k.z);

        /// <summary>Implicit conversion from UnityEngine.Vector3.</summary>
        public static implicit operator Keypoint3D(Vector3 v)
            => new Keypoint3D(v.x, v.y, v.z);

        public Vector3 ToVector3() => new Vector3(x, y, z);

        public override string ToString() => $"({x:F4}, {y:F4}, {z:F4})";
    }

    // ---------------------------------------------------------------------------
    // Complete hand pose with all 21 MediaPipe / XR-Hands keypoints.
    // ---------------------------------------------------------------------------
    [Serializable]
    public class HandPose
    {
        /// <summary>Array of exactly 21 keypoints.</summary>
        public Keypoint3D[] keypoints;

        /// <summary>Wrist orientation derived from IMU data.</summary>
        public Quaternion wristRotation;

        /// <summary>Raw wrist Euler angles (degrees) from IMU.</summary>
        public Vector3 wristEuler;

        /// <summary>Timestamp (seconds since epoch or since session start).</summary>
        public double timestamp;

        public HandPose()
        {
            keypoints = new Keypoint3D[21];
            wristRotation = Quaternion.identity;
            wristEuler = Vector3.zero;
            timestamp = 0;
        }

        /// <summary>Validate that the keypoint array is properly sized.</summary>
        public bool IsValid => keypoints != null && keypoints.Length == 21;
    }

    // ---------------------------------------------------------------------------
    // Finger enum matching the 5 fingers used in the data glove.
    // ---------------------------------------------------------------------------
    public enum FingerType
    {
        Thumb  = 0,
        Index  = 1,
        Middle = 2,
        Ring   = 3,
        Pinky  = 4
    }

    // ---------------------------------------------------------------------------
    // Per-finger joint angles (4 joints per finger, human hand convention).
    //   Joint 0 = MCP (metacarpophalangeal)
    //   Joint 1 = PIP (proximal interphalangeal)  — Thumb: CMC
    //   Joint 2 = DIP (distal interphalangeal)    — Thumb: MCP
    //   Joint 3 = Tip offset                       — Thumb: IP
    // ---------------------------------------------------------------------------
    [Serializable]
    public class FingerAngles
    {
        public FingerType finger;
        public float[] jointAngles; // 4 values in degrees

        public FingerAngles(FingerType finger)
        {
            this.finger = finger;
            jointAngles = new float[4];
        }

        /// <summary>Clamp all angles to their physiological limits.</summary>
        public void ClampToLimits(FingerJointLimits limits)
        {
            for (int i = 0; i < jointAngles.Length && i < limits.minAngles.Length; i++)
            {
                jointAngles[i] = Mathf.Clamp(
                    jointAngles[i],
                    limits.minAngles[i],
                    limits.maxAngles[i]
                );
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Joint rotation limits (degrees) per finger — based on human hand anatomy.
    // ---------------------------------------------------------------------------
    [Serializable]
    public class FingerJointLimits
    {
        public FingerType finger;
        public float[] minAngles = new float[4]; // MCP, PIP, DIP, Tip
        public float[] maxAngles = new float[4];

        /// <summary>Standard physiological limits for each finger.</summary>
        public static FingerJointLimits GetDefault(FingerType finger)
        {
            var limits = new FingerJointLimits { finger = finger };

            switch (finger)
            {
                case FingerType.Thumb:
                    limits.minAngles = new float[] {  0f, -20f,   0f,   0f };
                    limits.maxAngles = new float[] { 60f,  50f,  80f,  30f };
                    break;
                case FingerType.Index:
                    limits.minAngles = new float[] {  0f,   0f,   0f,   0f };
                    limits.maxAngles = new float[] { 90f, 100f,  80f,  20f };
                    break;
                case FingerType.Middle:
                    limits.minAngles = new float[] {  0f,   0f,   0f,   0f };
                    limits.maxAngles = new float[] { 90f, 100f,  80f,  20f };
                    break;
                case FingerType.Ring:
                    limits.minAngles = new float[] {  0f,   0f,   0f,   0f };
                    limits.maxAngles = new float[] { 90f, 100f,  80f,  20f };
                    break;
                case FingerType.Pinky:
                    limits.minAngles = new float[] {  0f,   0f,   0f,   0f };
                    limits.maxAngles = new float[] { 90f, 100f,  80f,  20f };
                    break;
            }

            return limits;
        }
    }

    // =========================================================================
    // SensorMessage — the JSON payload received from the Python Relay server.
    //
    // Expected JSON schema (from Python Relay):
    //   {
    //     "timestamp": 1700000000.123,
    //     "hall":      [0.12, 0.34, ..., 0.56],   // 15 flex sensor values
    //     "imu_euler": [0.1, -2.3, 45.6],         // roll, pitch, yaw (deg)
    //     "imu_gyro":  [1.0, -0.5, 0.3],          // angular velocity (deg/s)
    //     "keypoints": [{"x":0,"y":0,"z":0}, ...], // 21 keypoints (optional)
    //     "gesture":   "你好",
    //     "confidence": 0.95,
    //     "nlp_text":  "你好"
    //   }
    // =========================================================================
    [Serializable]
    public class SensorMessage
    {
        /// <summary>Server-side timestamp.</summary>
        public double timestamp;

        /// <summary>15 Hall-effect flex sensor ADC readings (normalized 0-1 or raw).</summary>
        public float[] hall;

        /// <summary>Wrist IMU Euler angles [roll, pitch, yaw] in degrees.</summary>
        public float[] imu_euler;

        /// <summary>Wrist IMU gyroscope [ωx, ωy, ωz] in deg/s.</summary>
        public float[] imu_gyro;

        /// <summary>Optional 21 keypoint positions from the inference pipeline.</summary>
        public Keypoint3D[] keypoints;

        /// <summary>Recognized gesture label (Chinese CSL or ASL).</summary>
        public string gesture;

        /// <summary>Gesture classification confidence [0, 1].</summary>
        public float confidence;

        /// <summary>NLP post-processed text output.</summary>
        public string nlp_text;

        // --- Convenience accessors -------------------------------------------

        public bool HasHallData => hall != null && hall.Length > 0;
        public bool HasIMUData => imu_euler != null && imu_euler.Length == 3;
        public bool HasGyroData => imu_gyro != null && imu_gyro.Length == 3;
        public bool HasKeypoints => keypoints != null && keypoints.Length == 21;
        public bool HasGesture => !string.IsNullOrEmpty(gesture);

        /// <summary>Validate basic message integrity.</summary>
        public bool IsValid()
        {
            // At minimum we need hall or keypoints
            return HasHallData || HasKeypoints;
        }

        // --- Factory: build a HandPose from this message ---------------------

        public HandPose ToHandPose()
        {
            var pose = new HandPose
            {
                timestamp = timestamp
            };

            if (HasKeypoints)
            {
                pose.keypoints = keypoints;
            }

            if (HasIMUData)
            {
                pose.wristEuler = new Vector3(imu_euler[0], imu_euler[1], imu_euler[2]);
                // Convert Euler to Quaternion (Unity convention: ZXY intrinsic)
                pose.wristRotation = Quaternion.Euler(pose.wristEuler);
            }

            return pose;
        }
    }

    // ---------------------------------------------------------------------------
    // Gesture classification result for the UI layer.
    // ---------------------------------------------------------------------------
    [Serializable]
    public struct GestureResult
    {
        public string label;
        public float confidence;
        public string nlpText;
        public double timestamp;

        public GestureResult(string label, float confidence, string nlpText, double ts)
        {
            this.label = label;
            this.confidence = confidence;
            this.nlpText = nlpText;
            this.timestamp = ts;
        }

        public static GestureResult Empty => new GestureResult("", 0f, "", 0);
    }

    // ---------------------------------------------------------------------------
    // Connection state for the networking layer.
    // ---------------------------------------------------------------------------
    public enum ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    }

    // ---------------------------------------------------------------------------
    // Event arguments passed when a new hand pose is available.
    // ---------------------------------------------------------------------------
    public class HandPoseEventArgs : EventArgs
    {
        public HandPose Pose { get; }
        public SensorMessage RawMessage { get; }

        public HandPoseEventArgs(HandPose pose, SensorMessage raw)
        {
            Pose = pose;
            RawMessage = raw;
        }
    }

    // ---------------------------------------------------------------------------
    // Event arguments for connection state changes.
    // ---------------------------------------------------------------------------
    public class ConnectionEventArgs : EventArgs
    {
        public ConnectionStatus Status { get; }
        public string Message { get; }

        public ConnectionEventArgs(ConnectionStatus status, string message = "")
        {
            Status = status;
            Message = message;
        }
    }
}
