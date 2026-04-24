// =============================================================================
// HandController.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// Main hand controller that orchestrates all finger controllers and wrist
// orientation. Receives SensorMessage from networking layer and distributes
// data to the appropriate sub-controllers.
//
// Keypoint layout (21 points, MediaPipe / XR-Hands convention):
//   0  = WRIST
//   1  = THUMB_CMC
//   2  = THUMB_MCP
//   3  = THUMB_IP
//   4  = THUMB_TIP
//   5  = INDEX_FINGER_MCP
//   6  = INDEX_FINGER_PIP
//   7  = INDEX_FINGER_DIP
//   8  = INDEX_FINGER_TIP
//   9  = MIDDLE_FINGER_MCP
//   10 = MIDDLE_FINGER_PIP
//   11 = MIDDLE_FINGER_DIP
//   12 = MIDDLE_FINGER_TIP
//   13 = RING_FINGER_MCP
//   14 = RING_FINGER_PIP
//   15 = RING_FINGER_DIP
//   16 = RING_FINGER_TIP
//   17 = PINKY_MCP
//   18 = PINKY_PIP
//   19 = PINKY_DIP
//   20 = PINKY_TIP
// =============================================================================

using System;
using System.Collections.Generic;
using UnityEngine;

namespace EdgeAI.DataGlove
{
    /// <summary>
    /// Central hand controller managing the full 21-keypoint hand skeleton.
    /// Supports two input modes:
    ///   1. Hall-sensor based: Maps 15 flex sensors to finger joint angles
    ///   2. Keypoint based: Directly positions 21 keypoints from inference
    /// </summary>
    [DefaultExecutionOrder(100)] // Run after networking receivers
    public class HandController : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("Hand Configuration")]
        [Tooltip("Left or right hand")]
        [SerializeField] private Handedness handedness = Handedness.Right;

        [Tooltip("Scale factor for the hand model")]
        [SerializeField] private float handScale = 1f;

        [Header("Keypoint Transforms")]
        [Tooltip("21 keypoint game objects in order (0=WRIST, ..., 20=PINKY_TIP)")]
        [SerializeField] private Transform[] keypointTransforms = new Transform[21];

        [Header("Finger Controllers")]
        [SerializeField] private FingerController thumbController;
        [SerializeField] private FingerController indexController;
        [SerializeField] private FingerController middleController;
        [SerializeField] private FingerController ringController;
        [SerializeField] private FingerController pinkyController;

        [Header("Wrist Transform")]
        [Tooltip("The wrist/root transform for IMU-based rotation")]
        [SerializeField] private Transform wristTransform;

        [Header("Input Mode")]
        [Tooltip("Hall-sensor mode: map 15 sensors to joint angles; Keypoint mode: direct positioning")]
        [SerializeField] private InputMode inputMode = InputMode.KeypointPriority;

        [Header("Smoothing")]
        [Tooltip("Wrist rotation smoothing factor (0 = instant, 0.95 = very smooth)")]
        [SerializeField] [Range(0f, 0.95f)] private float wristSmoothing = 0.25f;

        [Tooltip("Keypoint position smoothing factor")]
        [SerializeField] [Range(0f, 0.95f)] private float positionSmoothing = 0.3f;

        [Header("Hall Sensor Settings")]
        [Tooltip("Hall sensor value range for normalization [min, max]")]
        [SerializeField] private Vector2 hallSensorRange = new Vector2(0f, 1f);

        [Header("Debug")]
        [SerializeField] private bool showDebugInfo = true;
        [SerializeField] private bool showSkeletonLines = true;

        // -----------------------------------------------------------------------
        // Enums
        // -----------------------------------------------------------------------
        public enum Handedness { Left, Right }
        public enum InputMode { HallSensorOnly, KeypointOnly, KeypointPriority }

        // -----------------------------------------------------------------------
        // Public events
        // -----------------------------------------------------------------------
        /// <summary>Fired whenever the hand pose is updated.</summary>
        public event Action<HandPose> OnPoseUpdated;

        /// <summary>Fired when a gesture result is received.</summary>
        public event Action<GestureResult> OnGestureRecognized;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public bool IsReady { get; private set; }
        public HandPose CurrentPose { get; private set; } = new HandPose();
        public GestureResult CurrentGesture { get; private set; } = GestureResult.Empty;
        public Handedness Hand => handedness;
        public float UpdateRateHz { get; private set; }

        /// <summary>Dictionary mapping FingerType → FingerController for easy access.</summary>
        public Dictionary<FingerType, FingerController> FingerControllers { get; private set; }

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private Vector3[] _smoothedPositions = new Vector3[21];
        private Quaternion _smoothedWristRotation = Quaternion.identity;
        private Vector3[] _previousPositions = new Vector3[21];
        private int _updateCount;
        private float _rateAccumulator;
        private float _rateInterval = 1f;

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            InitializeFingerControllers();
            InitializeKeypoints();
        }

        private void Start()
        {
            ValidateSetup();
            ResetToDefaultPose();
            IsReady = true;
            Debug.Log($"[HandController] {handedness} hand initialized. Input mode: {inputMode}");
        }

        private void Update()
        {
            // Track update rate
            _rateAccumulator += Time.deltaTime;
            if (_rateAccumulator >= _rateInterval)
            {
                UpdateRateHz = _updateCount;
                _updateCount = 0;
                _rateAccumulator -= _rateInterval;
            }
        }

#if UNITY_EDITOR
        private void OnDrawGizmos()
        {
            if (!showSkeletonLines || keypointTransforms == null) return;

            // Draw bone connections
            Color boneColor = handedness == Handedness.Right ? Color.cyan : Color.orange;
            Gizmos.color = boneColor;

            int[][] bonePairs = GetBonePairs();
            foreach (var pair in bonePairs)
            {
                if (pair[0] < keypointTransforms.Length && pair[1] < keypointTransforms.Length
                    && keypointTransforms[pair[0]] != null && keypointTransforms[pair[1]] != null)
                {
                    Gizmos.DrawLine(
                        keypointTransforms[pair[0]].position,
                        keypointTransforms[pair[1]].position
                    );
                }
            }

            // Draw keypoint spheres
            for (int i = 0; i < keypointTransforms.Length && i < 21; i++)
            {
                if (keypointTransforms[i] != null)
                {
                    Gizmos.color = i == 0 ? Color.white : GetKeypointColor(i);
                    Gizmos.DrawSphere(keypointTransforms[i].position, 0.004f);
                }
            }
        }
#endif

        // -----------------------------------------------------------------------
        // Public methods — Primary update interface
        // -----------------------------------------------------------------------

        /// <summary>
        /// Main entry point: update hand pose from a SensorMessage.
        /// This is called by networking receivers (WebSocket or UDP).
        /// </summary>
        public void UpdateHandPose(SensorMessage message)
        {
            if (message == null || !message.IsValid())
            {
                Debug.LogWarning("[HandController] Received invalid SensorMessage.");
                return;
            }

            _updateCount++;

            // Choose input mode
            bool usedKeypoints = false;

            if (inputMode == InputMode.KeypointOnly
                || (inputMode == InputMode.KeypointPriority && message.HasKeypoints))
            {
                UpdateFromKeypoints(message);
                usedKeypoints = true;
            }

            if (!usedKeypoints && message.HasHallData)
            {
                UpdateFromHallSensors(message);
            }

            // Apply wrist rotation from IMU
            if (message.HasIMUData)
            {
                UpdateWristRotation(message.imu_euler, message.imu_gyro);
            }

            // Update pose object
            UpdateCurrentPose();

            // Handle gesture data
            if (message.HasGesture)
            {
                CurrentGesture = new GestureResult(
                    message.gesture,
                    message.confidence,
                    message.nlp_text,
                    message.timestamp
                );
                OnGestureRecognized?.Invoke(CurrentGesture);
            }

            OnPoseUpdated?.Invoke(CurrentPose);
        }

        /// <summary>Update hand pose from a pre-built HandPose object.</summary>
        public void UpdateHandPose(HandPose pose)
        {
            if (pose == null || !pose.IsValid) return;

            _updateCount++;

            if (pose.keypoints != null && pose.keypoints.Length == 21)
            {
                UpdateKeypointPositions(pose.keypoints);
            }

            if (pose.wristRotation != Quaternion.identity)
            {
                _smoothedWristRotation = Quaternion.Slerp(
                    _smoothedWristRotation,
                    pose.wristRotation,
                    1f - wristSmoothing
                );
                ApplyWristRotation(_smoothedWristRotation);
            }

            UpdateCurrentPose();
            OnPoseUpdated?.Invoke(CurrentPose);
        }

        // -----------------------------------------------------------------------
        // Hall-sensor based update
        // -----------------------------------------------------------------------
        private void UpdateFromHallSensors(SensorMessage message)
        {
            if (message.hall == null || message.hall.Length < 15)
            {
                Debug.LogWarning("[HandController] Insufficient Hall sensor data (need 15).");
                return;
            }

            // Distribute hall data to finger controllers
            if (thumbController  != null) thumbController.SetFromHallSensors(message.hall, hallSensorRange.x, hallSensorRange.y);
            if (indexController  != null) indexController.SetFromHallSensors(message.hall, hallSensorRange.x, hallSensorRange.y);
            if (middleController != null) middleController.SetFromHallSensors(message.hall, hallSensorRange.x, hallSensorRange.y);
            if (ringController   != null) ringController.SetFromHallSensors(message.hall, hallSensorRange.x, hallSensorRange.y);
            if (pinkyController  != null) pinkyController.SetFromHallSensors(message.hall, hallSensorRange.x, hallSensorRange.y);

            // If keypoints are also available, use them for position reference
            if (message.HasKeypoints)
            {
                UpdateKeypointPositions(message.keypoints);
            }
        }

        // -----------------------------------------------------------------------
        // Keypoint-based update
        // -----------------------------------------------------------------------
        private void UpdateFromKeypoints(SensorMessage message)
        {
            UpdateKeypointPositions(message.keypoints);
        }

        /// <summary>
        /// Update the 21 keypoint transform positions with smooth interpolation.
        /// </summary>
        public void UpdateKeypointPositions(Keypoint3D[] keypoints)
        {
            if (keypoints == null || keypoints.Length != 21) return;

            for (int i = 0; i < 21; i++)
            {
                Vector3 targetPos = keypoints[i].ToVector3() * handScale;

                // Apply handedness mirroring for left hand
                if (handedness == Handedness.Left)
                {
                    targetPos.x = -targetPos.x;
                }

                // Smooth interpolation
                _smoothedPositions[i] = Vector3.Lerp(
                    _smoothedPositions[i],
                    targetPos,
                    1f - positionSmoothing
                );

                // Apply to transform
                if (keypointTransforms != null && i < keypointTransforms.Length
                    && keypointTransforms[i] != null)
                {
                    keypointTransforms[i].position = _smoothedPositions[i];
                }
            }

            // Also extract finger bend angles from keypoint positions
            // and feed them to finger controllers for joint-limit enforcement
            ExtractFingerAnglesFromKeypoints(keypoints);
        }

        // -----------------------------------------------------------------------
        // Wrist rotation
        // -----------------------------------------------------------------------
        private void UpdateWristRotation(float[] imuEuler, float[] imuGyro)
        {
            if (imuEuler == null || imuEuler.Length < 3) return;

            // Convert IMU Euler to Quaternion
            // IMU convention: [roll(X), pitch(Y), yaw(Z)] in degrees
            // Unity convention: Euler(x, y, z) = pitch, yaw, roll
            Vector3 unityEuler = new Vector3(
                -imuEuler[1], // Pitch (negated for Unity coordinate system)
                imuEuler[2],  // Yaw
                -imuEuler[0]  // Roll (negated)
            );

            Quaternion targetRotation = Quaternion.Euler(unityEuler);

            // Apply handedness
            if (handedness == Handedness.Left)
            {
                targetRotation = new Quaternion(
                    targetRotation.x,
                    -targetRotation.y,
                    targetRotation.z,
                    -targetRotation.w
                );
            }

            // Smooth slerp
            _smoothedWristRotation = Quaternion.Slerp(
                _smoothedWristRotation,
                targetRotation,
                1f - wristSmoothing
            );

            ApplyWristRotation(_smoothedWristRotation);
        }

        private void ApplyWristRotation(Quaternion rotation)
        {
            if (wristTransform != null)
            {
                wristTransform.localRotation = rotation;
            }
            else if (keypointTransforms != null && keypointTransforms[0] != null)
            {
                keypointTransforms[0].localRotation = rotation;
            }
        }

        // -----------------------------------------------------------------------
        // Extract finger angles from keypoint positions (inverse kinematics lite)
        // -----------------------------------------------------------------------
        private void ExtractFingerAnglesFromKeypoints(Keypoint3D[] keypoints)
        {
            if (FingerControllers == null) return;

            // For each finger, compute angles between adjacent bone segments
            foreach (var kvp in FingerControllers)
            {
                int baseIdx = GetFingerBaseKeypointIndex(kvp.Key);
                if (baseIdx < 0) continue;

                Vector3 p0 = keypoints[baseIdx].ToVector3();     // MCP
                Vector3 p1 = keypoints[baseIdx + 1].ToVector3(); // PIP
                Vector3 p2 = keypoints[baseIdx + 2].ToVector3(); // DIP
                Vector3 p3 = keypoints[baseIdx + 3].ToVector3(); // Tip

                // Angle at MCP (between wrist-MCP and MCP-PIP)
                float mcpAngle = Vector3.Angle(
                    (keypoints[Math.Max(0, baseIdx - 1)].ToVector3() - p0).normalized,
                    (p1 - p0).normalized
                );

                // Angle at PIP (between MCP-PIP and PIP-DIP)
                float pipAngle = Vector3.Angle((p0 - p1).normalized, (p2 - p1).normalized);

                // Angle at DIP (between PIP-DIP and DIP-Tip)
                float dipAngle = Vector3.Angle((p1 - p2).normalized, (p3 - p2).normalized);

                // Convert to flexion (0 = straight, 90 = fully bent)
                mcpAngle = 90f - mcpAngle;
                pipAngle = 90f - pipAngle;
                dipAngle = 90f - dipAngle;

                kvp.Value.SetAngles(new float[] {
                    Mathf.Max(0f, mcpAngle),
                    Mathf.Max(0f, pipAngle),
                    Mathf.Max(0f, dipAngle),
                    Mathf.Max(0f, dipAngle * 0.3f)
                });
            }
        }

        /// <summary>
        /// Get the keypoint index of the MCP joint for a given finger.
        /// </summary>
        private int GetFingerBaseKeypointIndex(FingerType finger)
        {
            switch (finger)
            {
                case FingerType.Thumb:  return 1;  // THUMB_CMC
                case FingerType.Index:  return 5;  // INDEX_MCP
                case FingerType.Middle: return 9;  // MIDDLE_MCP
                case FingerType.Ring:   return 13; // RING_MCP
                case FingerType.Pinky:  return 17; // PINKY_MCP
                default: return -1;
            }
        }

        // -----------------------------------------------------------------------
        // State management
        // -----------------------------------------------------------------------
        private void UpdateCurrentPose()
        {
            CurrentPose = new HandPose
            {
                wristRotation = _smoothedWristRotation,
                timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds()
            };

            if (keypointTransforms != null)
            {
                CurrentPose.keypoints = new Keypoint3D[21];
                for (int i = 0; i < 21; i++)
                {
                    if (keypointTransforms[i] != null)
                    {
                        CurrentPose.keypoints[i] = new Keypoint3D(keypointTransforms[i].position);
                    }
                    else
                    {
                        CurrentPose.keypoints[i] = _smoothedPositions[i];
                    }
                }
            }
        }

        /// <summary>Reset all joints to default T-pose or flat hand pose.</summary>
        public void ResetToDefaultPose()
        {
            // Reset wrist
            _smoothedWristRotation = Quaternion.identity;
            ApplyWristRotation(Quaternion.identity);

            // Reset fingers
            if (thumbController  != null) thumbController.ResetToDefault();
            if (indexController  != null) indexController.ResetToDefault();
            if (middleController != null) middleController.ResetToDefault();
            if (ringController   != null) ringController.ResetToDefault();
            if (pinkyController  != null) pinkyController.ResetToDefault();

            // Reset keypoint positions
            for (int i = 0; i < 21; i++)
            {
                _smoothedPositions[i] = Vector3.zero;
            }

            Debug.Log($"[HandController] {handedness} hand reset to default pose.");
        }

        // -----------------------------------------------------------------------
        // Initialization
        // -----------------------------------------------------------------------
        private void InitializeFingerControllers()
        {
            FingerControllers = new Dictionary<FingerType, FingerController>
            {
                { FingerType.Thumb,  thumbController },
                { FingerType.Index,  indexController },
                { FingerType.Middle, middleController },
                { FingerType.Ring,   ringController },
                { FingerType.Pinky,  pinkyController }
            };
        }

        private void InitializeKeypoints()
        {
            _smoothedPositions = new Vector3[21];
            _previousPositions = new Vector3[21];

            for (int i = 0; i < 21; i++)
            {
                if (keypointTransforms != null && i < keypointTransforms.Length && keypointTransforms[i] != null)
                {
                    _smoothedPositions[i] = keypointTransforms[i].position;
                    _previousPositions[i] = keypointTransforms[i].position;
                }
            }
        }

        private void ValidateSetup()
        {
            bool hasErrors = false;

            // Check keypoints
            if (keypointTransforms == null || keypointTransforms.Length != 21)
            {
                Debug.LogError("[HandController] Need exactly 21 keypoint transforms assigned.");
                hasErrors = true;
            }
            else
            {
                for (int i = 0; i < 21; i++)
                {
                    if (keypointTransforms[i] == null)
                    {
                        Debug.LogWarning($"[HandController] Keypoint {i} ({GetKeypointName(i)}) is not assigned.");
                    }
                }
            }

            // Check finger controllers
            foreach (var kvp in FingerControllers)
            {
                if (kvp.Value == null)
                {
                    Debug.LogWarning($"[HandController] {kvp.Key} controller not assigned.");
                }
            }

            // Check wrist
            if (wristTransform == null && (keypointTransforms == null || keypointTransforms[0] == null))
            {
                Debug.LogWarning("[HandController] Wrist transform not assigned.");
            }

            if (!hasErrors)
            {
                Debug.Log("[HandController] All components validated successfully.");
            }
        }

        // -----------------------------------------------------------------------
        // Bone connection pairs (20 pairs for 21 keypoints)
        // -----------------------------------------------------------------------
        private static readonly int[][] BonePairs = new int[][]
        {
            // Wrist to each finger base
            new int[] { 0,  1 },  // Wrist → Thumb_CMC
            new int[] { 0,  5 },  // Wrist → Index_MCP
            new int[] { 0,  9 },  // Wrist → Middle_MCP
            new int[] { 0,  13 }, // Wrist → Ring_MCP
            new int[] { 0,  17 }, // Wrist → Pinky_MCP

            // Thumb chain
            new int[] { 1,  2 },  // Thumb_CMC → Thumb_MCP
            new int[] { 2,  3 },  // Thumb_MCP → Thumb_IP
            new int[] { 3,  4 },  // Thumb_IP  → Thumb_Tip

            // Index chain
            new int[] { 5,  6 },  // Index_MCP → Index_PIP
            new int[] { 6,  7 },  // Index_PIP → Index_DIP
            new int[] { 7,  8 },  // Index_DIP → Index_Tip

            // Middle chain
            new int[] { 9,  10 }, // Middle_MCP → Middle_PIP
            new int[] { 10, 11 }, // Middle_PIP → Middle_DIP
            new int[] { 11, 12 }, // Middle_DIP → Middle_Tip

            // Ring chain
            new int[] { 13, 14 }, // Ring_MCP → Ring_PIP
            new int[] { 14, 15 }, // Ring_PIP → Ring_DIP
            new int[] { 15, 16 }, // Ring_DIP → Ring_Tip

            // Pinky chain
            new int[] { 17, 18 }, // Pinky_MCP → Pinky_PIP
            new int[] { 18, 19 }, // Pinky_PIP → Pinky_DIP
            new int[] { 19, 20 }, // Pinky_DIP → Pinky_TIP
        };

        private static int[][] GetBonePairs() => BonePairs;

        // -----------------------------------------------------------------------
        // Debug helpers
        // -----------------------------------------------------------------------
        private static string GetKeypointName(int index)
        {
            string[] names = {
                "WRIST",
                "THUMB_CMC", "THUMB_MCP", "THUMB_IP", "THUMB_TIP",
                "INDEX_MCP", "INDEX_PIP", "INDEX_DIP", "INDEX_TIP",
                "MIDDLE_MCP", "MIDDLE_PIP", "MIDDLE_DIP", "MIDDLE_TIP",
                "RING_MCP", "RING_PIP", "RING_DIP", "RING_TIP",
                "PINKY_MCP", "PINKY_PIP", "PINKY_DIP", "PINKY_TIP"
            };
            return index >= 0 && index < names.Length ? names[index] : $"UNKNOWN_{index}";
        }

        private static Color GetKeypointColor(int index)
        {
            if (index == 0) return Color.white;
            if (index >= 1 && index <= 4)  return Color.red;
            if (index >= 5 && index <= 8)  return Color.green;
            if (index >= 9 && index <= 12) return Color.blue;
            if (index >= 13 && index <= 16) return Color.yellow;
            if (index >= 17 && index <= 20) return Color.magenta;
            return Color.gray;
        }
    }
}
