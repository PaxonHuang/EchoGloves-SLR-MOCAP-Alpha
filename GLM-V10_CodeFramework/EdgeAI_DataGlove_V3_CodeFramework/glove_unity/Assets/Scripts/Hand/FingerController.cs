// =============================================================================
// FingerController.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// Per-finger controller managing 4 joints (MCP, PIP, DIP, Tip) per finger.
// Maps Hall-effect sensor readings to anatomically correct joint rotations.
// =============================================================================

using System;
using UnityEngine;

namespace EdgeAI.DataGlove
{
    /// <summary>
    /// Controls a single finger's joint chain with anatomically correct rotation
    /// limits and sensor-to-angle mapping.
    /// </summary>
    [RequireComponent(typeof(Animator))]
    public class FingerController : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("Finger Configuration")]
        [SerializeField] private FingerType fingerType = FingerType.Index;

        [Tooltip("The 4 joint transforms in order: MCP → PIP → DIP → Tip")]
        [SerializeField] private Transform[] joints = new Transform[4];

        [Header("Sensor Mapping")]
        [Tooltip("Hall sensor indices (3 per finger): base, middle, tip")]
        [SerializeField] private int[] hallSensorIndices = new int[3];

        [Tooltip("Sensor value range for mapping [min, max] (normalized 0-1)")]
        [SerializeField] private Vector2 sensorRange = new Vector2(0f, 1f);

        [Tooltip("If true, higher sensor values = more curl (closed fist)")]
        [SerializeField] private bool invertedMapping = false;

        [Header("Angle Limits")]
        [Tooltip("Per-joint min angles (degrees): MCP, PIP, DIP, Tip")]
        [SerializeField] private float[] minAngles = new float[4];

        [Tooltip("Per-joint max angles (degrees): MCP, PIP, DIP, Tip")]
        [SerializeField] private float[] maxAngles = new float[4];

        [Header("Smoothing")]
        [Tooltip("Smoothing factor for angle interpolation (0 = no smoothing, 1 = no movement)")]
        [SerializeField] [Range(0f, 0.95f)] private float smoothing = 0.3f;

        [Header("Thumb-Specific")]
        [Tooltip("Thumb rotation axis (usually Z for adduction/abduction)")]
        [SerializeField] private Vector3 thumbAbductionAxis = new Vector3(0f, 0f, 1f);

        [Tooltip("Thumb abduction sensor index")]
        [SerializeField] private int thumbAbductionSensorIndex = -1;

        [Header("Debug")]
        [SerializeField] private bool showDebugGizmos = true;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        /// <summary>Current target joint angles (degrees).</summary>
        public float[] CurrentAngles { get; private set; } = new float[4];

        /// <summary>Smoothed joint angles being applied (degrees).</summary>
        public float[] SmoothedAngles { get; private set; } = new float[4];

        /// <summary>Whether all joint transforms are assigned.</summary>
        public bool IsFullyConfigured => joints != null && joints.Length == 4
            && joints[0] != null && joints[1] != null
            && joints[2] != null && joints[3] != null;

        public FingerType Finger => fingerType;

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private FingerJointLimits _limits;
        private bool _isInitialized;

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            InitializeLimits();
        }

        private void Start()
        {
            ValidateConfiguration();
            _isInitialized = true;
        }

        private void Update()
        {
            if (!_isInitialized) return;

            // Apply smoothed angles to joint transforms
            for (int i = 0; i < 4; i++)
            {
                if (joints[i] == null) continue;

                // Smooth interpolation
                SmoothedAngles[i] = Mathf.Lerp(SmoothedAngles[i], CurrentAngles[i], 1f - smoothing);

                // Convert to rotation and apply
                float angleDeg = SmoothedAngles[i];
                Vector3 rotationAxis = GetRotationAxis(i);
                joints[i].localRotation = Quaternion.AngleAxis(angleDeg, rotationAxis);
            }
        }

#if UNITY_EDITOR
        private void OnDrawGizmos()
        {
            if (!showDebugGizmos || joints == null) return;

            Gizmos.color = GetFingerColor(fingerType);

            for (int i = 0; i < joints.Length && joints[i] != null; i++)
            {
                Gizmos.DrawSphere(joints[i].position, 0.003f);

                if (i < joints.Length - 1 && joints[i + 1] != null)
                {
                    Gizmos.DrawLine(joints[i].position, joints[i + 1].position);
                }
            }
        }
#endif

        // -----------------------------------------------------------------------
        // Public methods
        // -----------------------------------------------------------------------

        /// <summary>
        /// Set finger joint angles directly (in degrees).
        /// </summary>
        /// <param name="angles">4-element array: MCP, PIP, DIP, Tip</param>
        public void SetAngles(float[] angles)
        {
            if (angles == null || angles.Length < 4)
            {
                Debug.LogWarning($"[FingerController] Invalid angle array for {fingerType}.");
                return;
            }

            for (int i = 0; i < 4; i++)
            {
                CurrentAngles[i] = Mathf.Clamp(angles[i], _limits.minAngles[i], _limits.maxAngles[i]);
            }
        }

        /// <summary>
        /// Set finger angles from Hall sensor readings.
        /// Maps 3 sensor values to 4 joint angles using a coupled model.
        /// </summary>
        /// <param name="hallData">Full hall sensor array (15 values)</param>
        /// <param name="sensorRangeMin">Sensor minimum (raw or normalized)</param>
        /// <param name="sensorRangeMax">Sensor maximum (raw or normalized)</param>
        public void SetFromHallSensors(float[] hallData, float sensorRangeMin = 0f, float sensorRangeMax = 1f)
        {
            if (hallData == null || hallData.Length < 15)
            {
                Debug.LogWarning("[FingerController] Insufficient Hall sensor data.");
                return;
            }

            // Read 3 sensor values for this finger
            float baseSensor = hallData[hallSensorIndices[0]];
            float midSensor  = hallData[hallSensorIndices[1]];
            float tipSensor  = hallData[hallSensorIndices[2]];

            // Normalize sensor values to [0, 1]
            float range = Mathf.Max(sensorRangeMax - sensorRangeMin, 0.001f);
            float baseNorm = Mathf.InverseLerp(sensorRangeMin, sensorRangeMax, baseSensor);
            float midNorm  = Mathf.InverseLerp(sensorRangeMin, sensorRangeMax, midSensor);
            float tipNorm  = Mathf.InverseLerp(sensorRangeMin, sensorRangeMax, tipSensor);

            if (invertedMapping)
            {
                baseNorm = 1f - baseNorm;
                midNorm  = 1f - midNorm;
                tipNorm  = 1f - tipNorm;
            }

            // Map to joint angles using coupled finger model
            // Based on human hand biomechanics:
            //   - MCP flexion correlates strongest with base sensor
            //   - PIP flexion is coupled with MCP (approximately 70% ratio)
            //   - DIP flexion is coupled with PIP (approximately 60% ratio)
            //   - Tip is a small add-on offset

            float mcpAngle = Mathf.Lerp(_limits.minAngles[0], _limits.maxAngles[0], baseNorm);
            float pipAngle = Mathf.Lerp(_limits.minAngles[1], _limits.maxAngles[1],
                Mathf.Max(midNorm, baseNorm * 0.7f)); // Coupling
            float dipAngle = Mathf.Lerp(_limits.minAngles[2], _limits.maxAngles[2],
                Mathf.Max(tipNorm, midNorm * 0.6f));   // Coupling
            float tipAngle = Mathf.Lerp(_limits.minAngles[3], _limits.maxAngles[3], tipNorm * 0.5f);

            CurrentAngles[0] = mcpAngle;
            CurrentAngles[1] = pipAngle;
            CurrentAngles[2] = dipAngle;
            CurrentAngles[3] = tipAngle;

            // Thumb abduction (special case)
            if (fingerType == FingerType.Thumb && thumbAbductionSensorIndex >= 0
                && thumbAbductionSensorIndex < hallData.Length)
            {
                float abdSensor = hallData[thumbAbductionSensorIndex];
                float abdNorm = Mathf.InverseLerp(sensorRangeMin, sensorRangeMax, abdSensor);
                // Thumb abduction is a special rotation applied to the CMC joint
                // This is handled separately in the thumb-specific update
            }
        }

        /// <summary>Reset all joints to zero rotation (flat hand).</summary>
        public void ResetToDefault()
        {
            for (int i = 0; i < 4; i++)
            {
                CurrentAngles[i] = 0f;
                SmoothedAngles[i] = 0f;
            }
        }

        /// <summary>Set smoothing factor at runtime.</summary>
        public void SetSmoothing(float value)
        {
            smoothing = Mathf.Clamp01(value);
        }

        // -----------------------------------------------------------------------
        // Initialization helpers
        // -----------------------------------------------------------------------
        private void InitializeLimits()
        {
            _limits = FingerJointLimits.GetDefault(fingerType);

            // Override with inspector values if set
            bool hasCustomMin = false, hasCustomMax = false;
            for (int i = 0; i < 4; i++)
            {
                if (minAngles != null && minAngles.Length == 4 && minAngles[i] != 0f)
                    hasCustomMin = true;
                if (maxAngles != null && maxAngles.Length == 4 && maxAngles[i] != 0f)
                    hasCustomMax = true;
            }

            if (hasCustomMin && minAngles != null)
                _limits.minAngles = (float[])minAngles.Clone();
            if (hasCustomMax && maxAngles != null)
                _limits.maxAngles = (float[])maxAngles.Clone();

            // Apply sensor range if set
            if (sensorRange.x != 0f || sensorRange.y != 1f)
            {
                sensorRange = new Vector2(sensorRange.x, Mathf.Max(sensorRange.y, sensorRange.x + 0.01f));
            }
        }

        private void ValidateConfiguration()
        {
            if (joints == null || joints.Length != 4)
            {
                Debug.LogError($"[FingerController] {fingerType}: Requires exactly 4 joint transforms.");
                return;
            }

            for (int i = 0; i < 4; i++)
            {
                if (joints[i] == null)
                {
                    Debug.LogWarning($"[FingerController] {fingerType}: Joint {i} is not assigned.");
                }
            }

            // Validate sensor indices
            if (hallSensorIndices == null || hallSensorIndices.Length != 3)
            {
                Debug.LogWarning($"[FingerController] {fingerType}: Requires exactly 3 Hall sensor indices.");
                SetDefaultSensorIndices();
            }

            for (int i = 0; i < hallSensorIndices.Length; i++)
            {
                if (hallSensorIndices[i] < 0 || hallSensorIndices[i] >= 15)
                {
                    Debug.LogError($"[FingerController] {fingerType}: Invalid sensor index {hallSensorIndices[i]}.");
                }
            }
        }

        /// <summary>
        /// Set default Hall sensor indices based on finger type.
        /// Glove sensor layout (15 channels):
        ///   Thumb:     [0, 1, 2]  (CMC, MCP, IP)
        ///   Index:     [3, 4, 5]  (MCP, PIP, DIP)
        ///   Middle:    [6, 7, 8]  (MCP, PIP, DIP)
        ///   Ring:      [9, 10, 11] (MCP, PIP, DIP)
        ///   Pinky:     [12, 13, 14] (MCP, PIP, DIP)
        /// </summary>
        private void SetDefaultSensorIndices()
        {
            int baseIdx = (int)fingerType * 3;
            hallSensorIndices = new int[] { baseIdx, baseIdx + 1, baseIdx + 2 };
        }

        // -----------------------------------------------------------------------
        // Rotation axis per joint
        // -----------------------------------------------------------------------
        private Vector3 GetRotationAxis(int jointIndex)
        {
            switch (fingerType)
            {
                case FingerType.Thumb:
                    // Thumb has more complex rotations
                    switch (jointIndex)
                    {
                        case 0: return Vector3.forward; // CMC - abduction/adduction
                        case 1: return Vector3.right;   // MCP - flexion
                        case 2: return Vector3.right;   // IP  - flexion
                        case 3: return Vector3.right;   // Tip
                        default: return Vector3.right;
                    }

                default:
                    // Four fingers: primary flexion is around the X axis
                    // MCP also has slight abduction (Z axis) but simplified here
                    switch (jointIndex)
                    {
                        case 0: return Vector3.right;   // MCP - flexion
                        case 1: return Vector3.right;   // PIP - flexion
                        case 2: return Vector3.right;   // DIP - flexion
                        case 3: return Vector3.right;   // Tip
                        default: return Vector3.right;
                    }
            }
        }

        // -----------------------------------------------------------------------
        // Debug helpers
        // -----------------------------------------------------------------------
        private static Color GetFingerColor(FingerType finger)
        {
            switch (finger)
            {
                case FingerType.Thumb:  return Color.red;
                case FingerType.Index:  return Color.green;
                case FingerType.Middle: return Color.blue;
                case FingerType.Ring:   return Color.yellow;
                case FingerType.Pinky:  return Color.magenta;
                default: return Color.white;
            }
        }

        /// <summary>Get a human-readable summary of current finger state.</summary>
        public string GetDebugInfo()
        {
            string s = $"{fingerType}: ";
            for (int i = 0; i < 4; i++)
            {
                s += $"J{i}={SmoothedAngles[i]:F1}° ";
            }
            return s.Trim();
        }
    }
}
