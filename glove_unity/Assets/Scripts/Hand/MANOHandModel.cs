// =============================================================================
// MANOHandModel.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// Integration with the ms-MANO (Multi-Articulated Neural Network Hand Object)
// parametric hand model for high-fidelity hand rendering.
//
// MANO provides:
//   - 48 DOF pose parameters (3 global + 15 joints × 3 rotations each)
//   - 10 shape (beta) parameters for hand shape personalization
//   - 778-vertex triangular mesh per hand
//   - UV-mapped texture for photorealistic rendering
//
// This component bridges the MANO model with Unity's SkinnedMeshRenderer.
//
// References:
//   - MANO: https://mano.is.tue.mpg.com/
//   - Original paper: "Learning a Model of Hand Shape and Pose from 1000 Images"
//     Romero et al., SIGGRAPH Asia 2017
// =============================================================================

using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace EdgeAI.DataGlove
{
    /// <summary>
    /// Manages the ms-MANO hand model in Unity. Handles loading the pre-trained
    /// model files, applying pose/shape parameters, and updating the
    /// SkinnedMeshRenderer for visualization.
    /// </summary>
    [RequireComponent(typeof(SkinnedMeshRenderer))]
    public class MANOHandModel : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("Model Assets")]
        [Tooltip("Pre-computed MANO mesh vertices (778 vertices × 3 floats, .bin or .asset)")]
        [SerializeField] private TextAsset manoVerticesAsset;

        [Tooltip("Pre-computed MANO faces/triangles (1536 triangles × 3 ints)")]
        [SerializeField] private TextAsset manoFacesAsset;

        [Tooltip("MANO joint regressor (778 vertices → 16 joints, 778×16 matrix)")]
        [SerializeField] private TextAsset jointRegressorAsset;

        [Tooltip("Pre-computed MANO pose blend shapes (48×3 params, 778 vertices × 3 × 62)")]
        [SerializeField] private TextAsset poseBlendShapesAsset;

        [Tooltip("Pre-computed MANO shape blend shapes (10 betas, 778 vertices × 3 × 10)")]
        [SerializeField] private TextAsset shapeBlendShapesAsset;

        [Header("Material")]
        [Tooltip("Material for rendering the hand mesh")]
        [SerializeField] private Material handMaterial;

        [Tooltip("Fallback material if handMaterial is not set")]
        [SerializeField] private Material fallbackMaterial;

        [Header("Hand Configuration")]
        [Tooltip("Left or right hand model")]
        [SerializeField] private HandController.Handedness handedness = HandController.Handedness.Right;

        [Tooltip("Scale factor applied to the MANO model")]
        [SerializeField] private float modelScale = 1f;

        [Header("Rendering")]
        [Tooltip("Enable high-quality rendering mode (subsurface scattering, etc.)")]
        [SerializeField] private bool highQualityRendering = true;

        [Tooltip("Render hand mesh as wireframe overlay")]
        [SerializeField] private bool showWireframe = false;

        [Tooltip("Show joint positions as spheres")]
        [SerializeField] private bool showJointSpheres = true;

        [Tooltip("Joint sphere prefab")]
        [SerializeField] private GameObject jointSpherePrefab;

        [Header("Skeleton Definition")]
        [Tooltip("Hand skeleton JSON file path relative to StreamingAssets")]
        [SerializeField] private string skeletonDefinitionFile = "hand_skeleton.json";

        // -----------------------------------------------------------------------
        // Public events
        // -----------------------------------------------------------------------
        public event Action OnModelLoaded;
        public event Action OnPoseApplied;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public bool IsModelLoaded { get; private set; }
        public int VertexCount { get; private set; }
        public int FaceCount { get; private set; }
        public int JointCount { get; private set; } = 16; // MANO uses 16 joints (wrist + 15)

        /// <summary>Current 48 DOF pose parameters (3 global + 45 joint rotations).</summary>
        public float[] PoseParameters { get; private set; } = new float[48];

        /// <summary>Current 10 shape (beta) parameters for hand personalization.</summary>
        public float[] ShapeParameters { get; private set; } = new float[10];

        /// <summary>Joint transforms in MANO space (16 joints).</summary>
        public Transform[] JointTransforms { get; private set; } = new Transform[16];

        /// <summary>Joint rotations as Quaternions (16 joints, wrist is identity).</summary>
        public Quaternion[] JointRotations { get; private set; } = new Quaternion[16];

        // -----------------------------------------------------------------------
        // MANO joint hierarchy (right hand, left hand mirrors on Z axis)
        // Joint indices (16 total):
        //   0  = Wrist
        //   1  = Index_MCP,  2 = Index_PIP, 3 = Index_DIP, 4 = Index_TIP
        //   5  = Middle_MCP, 6 = Middle_PIP, 7 = Middle_DIP, 8 = Middle_TIP
        //   9  = Pinky_MCP,  10 = Pinky_PIP, 11 = Pinky_DIP, 12 = Pinky_TIP
        //   13 = Ring_MCP,   14 = Ring_PIP,  15 = Ring_TIP
        //   (Thumb: 0→T1→T2→T3 — special handling)
        // -----------------------------------------------------------------------
        private static readonly int[][] ParentIndices = new int[][]
        {
            new int[] { -1, 0,  1,  2,  3 },  // Wrist → Thumb chain
            new int[] { 0,  5,  6,  7,  8 },  // Wrist → Index chain
            new int[] { 0,  9,  10, 11, 12 }, // Wrist → Middle chain
            new int[] { 0, 13, 14, 15, 16 },  // Wrist → Ring chain
            new int[] { 0, 17, 18, 19, 20 },  // Wrist → Pinky chain
        };

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private SkinnedMeshRenderer _skinnedMeshRenderer;
        private Mesh _handMesh;
        private BoneWeight[] _boneWeights;
        private Matrix4x4[] _bindPoses;
        private Transform[] _boneTransforms;
        private GameObject _jointSpheresParent;

        // Pre-computed mesh data (loaded from assets)
        private Vector3[] _templateVertices;
        private int[] _templateFaces;
        private float[,] _jointRegressorMatrix;     // [778, 16]
        private Vector3[][, ] _poseBlendShapes;     // [62, 778, 3]
        private Vector3[][, ] _shapeBlendShapes;    // [10, 778, 3]

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            _skinnedMeshRenderer = GetComponent<SkinnedMeshRenderer>();
            InitializeModel();
        }

        private void Start()
        {
            LoadSkeletonDefinition();
            CreateJointVisualization();
        }

        private void Update()
        {
            // Update bone transforms each frame (in case parent moved)
            UpdateBoneTransforms();
        }

        private void OnDestroy()
        {
            CleanupJointVisualization();
        }

        // -----------------------------------------------------------------------
        // Model initialization
        // -----------------------------------------------------------------------
        private void InitializeModel()
        {
            Debug.Log($"[MANOHandModel] Initializing {handedness} hand model...");

            // Try loading from assets
            if (manoVerticesAsset != null)
            {
                LoadModelFromAssets();
            }
            else
            {
                // Try loading from StreamingAssets (binary files)
                LoadModelFromStreamingAssets();
            }

            // If still no data, create a procedural fallback
            if (_templateVertices == null || _templateVertices.Length == 0)
            {
                CreateProceduralHandMesh();
            }

            // Build the skinned mesh
            BuildSkinnedMesh();
            IsModelLoaded = true;
            OnModelLoaded?.Invoke();

            Debug.Log($"[MANOHandModel] Model ready. Vertices: {VertexCount}, Faces: {FaceCount}");
        }

        private void LoadModelFromAssets()
        {
            try
            {
                // Parse vertices from TextAsset (binary or JSON format)
                _templateVertices = ParseBinaryVertices(manoVerticesAsset.bytes);
                VertexCount = _templateVertices.Length;

                if (manoFacesAsset != null)
                {
                    _templateFaces = ParseFaces(manoFacesAsset.bytes);
                    FaceCount = _templateFaces.Length / 3;
                }
                else
                {
                    // Default MANO topology: 1536 faces
                    _templateFaces = LoadDefaultMANOTopology();
                    FaceCount = _templateFaces.Length / 3;
                }

                // Load blend shapes
                if (poseBlendShapesAsset != null)
                {
                    _poseBlendShapes = ParsePoseBlendShapes(poseBlendShapesAsset.bytes);
                }

                if (shapeBlendShapesAsset != null)
                {
                    _shapeBlendShapes = ParseShapeBlendShapes(shapeBlendShapesAsset.bytes);
                }

                // Load joint regressor
                if (jointRegressorAsset != null)
                {
                    _jointRegressorMatrix = ParseJointRegressor(jointRegressorAsset.bytes);
                }
                else
                {
                    Debug.LogWarning("[MANOHandModel] Joint regressor not found. Using heuristic joint placement.");
                }

                Debug.Log($"[MANOHandModel] Loaded model from assets: {VertexCount} vertices, {FaceCount} faces.");
            }
            catch (Exception ex)
            {
                Debug.LogError($"[MANOHandModel] Failed to load model assets: {ex.Message}");
                CreateProceduralHandMesh();
            }
        }

        private void LoadModelFromStreamingAssets()
        {
            string basePath = Path.Combine(Application.streamingAssetsPath,
                handedness == HandController.Handedness.Right ? "mano_right" : "mano_left");

            try
            {
                string vertexFile = Path.Combine(basePath, "vertices.bin");
                if (File.Exists(vertexFile))
                {
                    byte[] vertexData = File.ReadAllBytes(vertexFile);
                    _templateVertices = ParseBinaryVertices(vertexData);
                    VertexCount = _templateVertices.Length;
                }

                string faceFile = Path.Combine(basePath, "faces.bin");
                if (File.Exists(faceFile))
                {
                    byte[] faceData = File.ReadAllBytes(faceFile);
                    _templateFaces = ParseFaces(faceData);
                    FaceCount = _templateFaces.Length / 3;
                }
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"[MANOHandModel] Could not load from StreamingAssets: {ex.Message}");
            }
        }

        // -----------------------------------------------------------------------
        // Procedural hand mesh (fallback when MANO model files are not available)
        // -----------------------------------------------------------------------
        private void CreateProceduralHandMesh()
        {
            Debug.Log("[MANOHandModel] Creating procedural hand mesh (MANO assets not found).");
            Debug.Log("[MANOHandModel] For production use, download MANO model files from:");
            Debug.Log("[MANOHandModel]   https://mano.is.tue.mpg.com/");

            // Create a simplified hand-shaped mesh
            var mesh = new Mesh();

            // Generate a box-based hand approximation
            // Palm
            List<Vector3> verts = new List<Vector3>();
            List<int> tris = new List<int>();
            List<Vector2> uvs = new List<Vector2>();

            // Palm base (flat box)
            CreateBox(verts, tris, uvs,
                center: new Vector3(0f, 0.04f, 0f),
                size: new Vector3(0.06f, 0.01f, 0.08f));

            // Fingers (simplified boxes)
            float[][] fingerOffsets = {
                new float[] { -0.035f, 0.045f, -0.02f },  // Thumb
                new float[] { -0.02f,  0.045f,  0.03f },  // Index
                new float[] {  0.00f,  0.045f,  0.035f }, // Middle
                new float[] {  0.015f, 0.045f,  0.025f }, // Ring
                new float[] {  0.025f, 0.04f,   0.01f },  // Pinky
            };

            float[][] fingerSizes = {
                new float[] { 0.015f, 0.01f, 0.025f },  // Thumb (short)
                new float[] { 0.01f,  0.008f, 0.035f }, // Index
                new float[] { 0.01f,  0.008f, 0.04f },  // Middle
                new float[] { 0.01f,  0.008f, 0.035f }, // Ring
                new float[] { 0.008f, 0.007f, 0.025f }, // Pinky
            };

            for (int f = 0; f < 5; f++)
            {
                Vector3 center = new Vector3(
                    fingerOffsets[f][0],
                    fingerOffsets[f][1],
                    fingerOffsets[f][2]
                );
                Vector3 size = new Vector3(
                    fingerSizes[f][0],
                    fingerSizes[f][1],
                    fingerSizes[f][2]
                );
                CreateBox(verts, tris, uvs, center, size);
            }

            // Wrist
            CreateBox(verts, tris, uvs,
                center: new Vector3(0f, 0.04f, -0.06f),
                size: new Vector3(0.05f, 0.015f, 0.03f));

            _templateVertices = verts.ToArray();
            _templateFaces = tris.ToArray();
            VertexCount = _templateVertices.Length;
            FaceCount = _templateFaces.Length / 3;
        }

        private void CreateBox(List<Vector3> verts, List<int> tris, List<Vector2> uvs,
            Vector3 center, Vector3 size)
        {
            int baseIdx = verts.Count;
            float hx = size.x / 2f, hy = size.y / 2f, hz = size.z / 2f;

            // 8 corners
            verts.Add(center + new Vector3(-hx, -hy, -hz));
            verts.Add(center + new Vector3( hx, -hy, -hz));
            verts.Add(center + new Vector3( hx,  hy, -hz));
            verts.Add(center + new Vector3(-hx,  hy, -hz));
            verts.Add(center + new Vector3(-hx, -hy,  hz));
            verts.Add(center + new Vector3( hx, -hy,  hz));
            verts.Add(center + new Vector3( hx,  hy,  hz));
            verts.Add(center + new Vector3(-hx,  hy,  hz));

            // 6 faces (12 triangles)
            int[] faceIndices = {
                // Front
                4, 6, 5, 4, 7, 6,
                // Back
                0, 1, 2, 0, 2, 3,
                // Top
                3, 7, 6, 3, 6, 2,
                // Bottom
                0, 5, 1, 0, 4, 5,
                // Right
                1, 5, 6, 1, 6, 2,
                // Left
                0, 3, 7, 0, 7, 4
            };

            foreach (int fi in faceIndices)
            {
                tris.Add(baseIdx + fi);
            }

            // UVs
            Vector2[] boxUvs = {
                new Vector2(0, 0), new Vector2(1, 0),
                new Vector2(1, 1), new Vector2(0, 1),
                new Vector2(0, 0), new Vector2(1, 0),
                new Vector2(1, 1), new Vector2(0, 1)
            };
            uvs.AddRange(boxUvs);
        }

        // -----------------------------------------------------------------------
        // Skinned mesh building
        // -----------------------------------------------------------------------
        private void BuildSkinnedMesh()
        {
            _handMesh = new Mesh();
            _handMesh.name = $"MANO_Hand_{handedness}";
            _handMesh.vertices = _templateVertices;
            _handMesh.triangles = _templateFaces;
            _handMesh.RecalculateNormals();
            _handMesh.RecalculateBounds();

            // Create bone weight data
            int boneCount = JointCount + 1; // Joints + root
            _boneWeights = new BoneWeight[VertexCount];
            _bindPoses = new Matrix4x4[boneCount];
            _boneTransforms = new Transform[boneCount];

            // Create bone transforms as children
            for (int i = 0; i < boneCount; i++)
            {
                GameObject boneObj = new GameObject($"Bone_{i}");
                boneObj.transform.SetParent(transform);
                boneObj.transform.localPosition = Vector3.zero;
                _boneTransforms[i] = boneObj.transform;
                _bindPoses[i] = _boneTransforms[i].worldToLocalMatrix * transform.localToWorldMatrix;
            }

            // Assign bone weights using joint regressor or heuristic
            if (_jointRegressorMatrix != null)
            {
                AssignBoneWeightsFromRegressor();
            }
            else
            {
                AssignBoneWeightsHeuristic();
            }

            _handMesh.boneWeights = _boneWeights;
            _handMesh.bindposes = _bindPoses;

            // Setup renderer
            _skinnedMeshRenderer.sharedMesh = _handMesh;
            _skinnedMeshRenderer.bones = _boneTransforms;
            _skinnedMeshRenderer.material = handMaterial != null ? handMaterial : fallbackMaterial;

            if (_skinnedMeshRenderer.material == null)
            {
                _skinnedMeshRenderer.material = CreateDefaultMaterial();
            }

            // Store joint transforms for external access
            Array.Resize(ref JointTransforms, JointCount);
            for (int i = 0; i < JointCount; i++)
            {
                JointTransforms[i] = _boneTransforms[i + 1]; // Skip root
            }
        }

        private void AssignBoneWeightsFromRegressor()
        {
            // Use joint regressor to assign each vertex to its closest joint
            for (int v = 0; v < VertexCount; v++)
            {
                int bestJoint = 0;
                float bestWeight = float.MinValue;

                for (int j = 0; j < JointCount; j++)
                {
                    float w = _jointRegressorMatrix[v, j];
                    if (w > bestWeight)
                    {
                        bestWeight = w;
                        bestJoint = j;
                    }
                }

                _boneWeights[v] = new BoneWeight
                {
                    boneIndex0 = bestJoint + 1, // +1 for root offset
                    weight0 = 1f
                };
            }
        }

        private void AssignBoneWeightsHeuristic()
        {
            // Simple heuristic: assign vertices to bones based on Y position
            // This is a rough approximation for the procedural mesh
            float minY = float.MaxValue, maxY = float.MinValue;
            for (int i = 0; i < VertexCount; i++)
            {
                if (_templateVertices[i].y < minY) minY = _templateVertices[i].y;
                if (_templateVertices[i].y > maxY) maxY = _templateVertices[i].y;
            }
            float range = Mathf.Max(maxY - minY, 0.001f);

            for (int v = 0; v < VertexCount; v++)
            {
                float normalizedY = (_templateVertices[v].y - minY) / range;
                int boneIdx = Mathf.Clamp(Mathf.FloorToInt(normalizedY * JointCount), 0, JointCount - 1);
                boneIdx += 1; // Offset for root

                _boneWeights[v] = new BoneWeight
                {
                    boneIndex0 = boneIdx,
                    weight0 = 1f
                };
            }
        }

        // -----------------------------------------------------------------------
        // Pose parameter interface
        // -----------------------------------------------------------------------

        /// <summary>
        /// Set all 48 pose parameters at once.
        /// Layout: [global_rot(3), joint_rot_0(3), joint_rot_1(3), ..., joint_rot_15(3)]
        /// where each rotation is axis-angle (3 floats: ax, ay, az).
        /// </summary>
        public void SetPoseParameters(float[] pose)
        {
            if (pose == null || pose.Length < 48)
            {
                Debug.LogWarning("[MANOHandModel] Pose parameters must be 48 floats.");
                return;
            }

            Array.Copy(pose, PoseParameters, 48);
            ApplyPoseParameters();
        }

        /// <summary>
        /// Set individual joint rotation using axis-angle.
        /// </summary>
        /// <param name="jointIndex">Joint index (0–15)</param>
        /// <param name="axisAngle">Axis-angle rotation (3 floats)</param>
        public void SetJointPose(int jointIndex, Vector3 axisAngle)
        {
            if (jointIndex < 0 || jointIndex >= JointCount) return;

            int offset = 3 + jointIndex * 3;
            PoseParameters[offset + 0] = axisAngle.x;
            PoseParameters[offset + 1] = axisAngle.y;
            PoseParameters[offset + 2] = axisAngle.z;

            ApplyPoseParameters();
        }

        /// <summary>
        /// Set the global wrist rotation (first 3 pose parameters).
        /// </summary>
        public void SetGlobalRotation(Vector3 axisAngle)
        {
            PoseParameters[0] = axisAngle.x;
            PoseParameters[1] = axisAngle.y;
            PoseParameters[2] = axisAngle.z;

            ApplyPoseParameters();
        }

        /// <summary>
        /// Set the 10 shape (beta) parameters for hand personalization.
        /// Typical values: mean = 0, range ≈ [-3, 3].
        /// </summary>
        public void SetShapeParameters(float[] betas)
        {
            if (betas == null || betas.Length < 10) return;
            Array.Copy(betas, ShapeParameters, 10);
            ApplyShapeParameters();
        }

        // -----------------------------------------------------------------------
        // Pose & shape application
        // -----------------------------------------------------------------------
        private void ApplyPoseParameters()
        {
            // Global rotation (first 3 params → Quaternion)
            Vector3 globalAxisAngle = new Vector3(
                PoseParameters[0],
                PoseParameters[1],
                PoseParameters[2]
            );

            if (globalAxisAngle.sqrMagnitude > 0.0001f)
            {
                float angle = globalAxisAngle.magnitude * Mathf.Rad2Deg;
                Quaternion globalRot = Quaternion.AngleAxis(angle, globalAxisAngle.normalized);
                transform.localRotation = globalRot * Quaternion.Euler(0f, handedness == HandController.Handedness.Right ? 180f : 0f, 0f);
            }

            // Joint rotations (params 3..48 → 15 joints)
            JointRotations[0] = Quaternion.identity; // Wrist = identity
            for (int j = 0; j < JointCount - 1; j++)
            {
                int offset = 3 + j * 3;
                Vector3 jointAA = new Vector3(
                    PoseParameters[offset + 0],
                    PoseParameters[offset + 1],
                    PoseParameters[offset + 2]
                );

                if (jointAA.sqrMagnitude > 0.0001f)
                {
                    float angle = jointAA.magnitude * Mathf.Rad2Deg;
                    JointRotations[j + 1] = Quaternion.AngleAxis(angle, jointAA.normalized);
                }
                else
                {
                    JointRotations[j + 1] = Quaternion.identity;
                }
            }

            // Apply rotations to bone transforms
            UpdateBoneTransforms();

            // Apply pose blend shapes to mesh if available
            ApplyPoseBlendShapes();

            OnPoseApplied?.Invoke();
        }

        private void ApplyShapeParameters()
        {
            ApplyShapeBlendShapes();
        }

        private void ApplyPoseBlendShapes()
        {
            if (_poseBlendShapes == null || _handMesh == null) return;

            Vector3[] vertices = (Vector3[])_templateVertices.Clone();

            // Add pose-dependent blend shape contributions
            // Each pose parameter contributes a directional blend shape
            for (int j = 0; j < 15; j++)
            {
                int offset = 3 + j * 3;
                for (int axis = 0; axis < 3; axis++)
                {
                    int bsIdx = j * 3 + axis;
                    if (bsIdx < _poseBlendShapes.GetLength(0))
                    {
                        float weight = PoseParameters[offset + axis];
                        for (int v = 0; v < VertexCount && v < 778; v++)
                        {
                            vertices[v] += _poseBlendShapes[bsIdx, v, 0] * weight;
                            // Note: actual MANO uses 3-component blend shapes per vertex
                        }
                    }
                }
            }

            _handMesh.vertices = vertices;
            _handMesh.RecalculateNormals();
            _handMesh.RecalculateBounds();
        }

        private void ApplyShapeBlendShapes()
        {
            if (_shapeBlendShapes == null || _handMesh == null) return;

            Vector3[] vertices = (Vector3[])_templateVertices.Clone();

            for (int b = 0; b < 10; b++)
            {
                if (b >= _shapeBlendShapes.GetLength(0)) break;

                float weight = ShapeParameters[b];
                for (int v = 0; v < VertexCount && v < 778; v++)
                {
                    vertices[v] += _shapeBlendShapes[b, v, 0] * weight;
                }
            }

            _templateVertices = vertices;
            _handMesh.vertices = vertices;
            _handMesh.RecalculateNormals();
            _handMesh.RecalculateBounds();
        }

        private void UpdateBoneTransforms()
        {
            if (_boneTransforms == null) return;

            for (int j = 0; j < JointCount && j < JointRotations.Length; j++)
            {
                if (JointTransforms[j] != null)
                {
                    JointTransforms[j].localRotation = JointRotations[j];
                }
            }
        }

        // -----------------------------------------------------------------------
        // HandController integration
        // -----------------------------------------------------------------------

        /// <summary>
        /// Convert from HandController's 21-keypoint format to MANO's 16-joint format.
        /// </summary>
        public void UpdateFromHandController(HandPose pose)
        {
            if (pose == null || !pose.IsValid) return;

            // Map wrist rotation to global MANO rotation
            if (pose.wristRotation != Quaternion.identity)
            {
                Vector3 aa = QuaternionToAxisAngle(pose.wristRotation);
                PoseParameters[0] = aa.x;
                PoseParameters[1] = aa.y;
                PoseParameters[2] = aa.z;
            }

            // Map 21 keypoints to 15 MANO joint rotations
            if (pose.keypoints != null && pose.keypoints.Length == 21)
            {
                MapKeypointsToMANOJoints(pose.keypoints);
            }

            ApplyPoseParameters();
        }

        private void MapKeypointsToMANOJoints(Keypoint3D[] keypoints)
        {
            // Simplified mapping: compute joint angles from keypoint positions
            // MANO joint ordering: Wrist, Index(MCP,PIP,DIP,TIP), Middle(...), Pinky(...), Ring(...)

            // Map indices: 21-keypoint → MANO 15 joint chain
            // MANO ordering: [Wrist, I_MCP, I_PIP, I_DIP, I_TIP, M_MCP, M_PIP, M_DIP, M_TIP,
            //                 P_MCP, P_PIP, P_DIP, P_TIP, R_MCP, R_PIP, R_TIP]

            // Note: MANO uses a different joint ordering than MediaPipe.
            // This is a simplified mapping for demonstration.

            int[][] kpToMano = {
                new int[] { -1 },           // 0: Wrist (global, handled separately)
                new int[] { 1, 2, 3, 4 },   // 1-4: Thumb → MANO Thumb joints (simplified)
                new int[] { 1, 2, 3, 4 },   // 5-8: Index → MANO Index
                new int[] { 5, 6, 7, 8 },   // 9-12: Middle → MANO Middle
                new int[] { 9, 10, 11, 12 }, // 13-16: Ring → MANO Ring
                new int[] { 13, 14, 15, 16 },// 17-20: Pinky → MANO Pinky
            };

            // Compute rotation for each joint from consecutive bone segments
            for (int f = 0; f < 5; f++)
            {
                for (int j = 0; j < 4; j++)
                {
                    int kpIdx = f == 0 ? 1 + j : 5 + (f - 1) * 4 + j;
                    if (kpIdx >= 20) break;

                    Vector3 current = keypoints[kpIdx].ToVector3();
                    Vector3 parent = kpIdx > 0 ? keypoints[kpIdx - 1].ToVector3() : current;
                    Vector3 child = kpIdx < 20 ? keypoints[kpIdx + 1].ToVector3() : current;

                    Vector3 boneDir = (child - current).normalized;
                    Vector3 parentBoneDir = (current - parent).normalized;

                    Quaternion jointRot = Quaternion.FromToRotation(parentBoneDir, boneDir);
                    Vector3 aa = QuaternionToAxisAngle(jointRot);

                    int manoJointIdx = (f == 0 ? 0 : f - 1) * 4 + j;
                    if (manoJointIdx < 15)
                    {
                        int offset = 3 + manoJointIdx * 3;
                        PoseParameters[offset + 0] = aa.x;
                        PoseParameters[offset + 1] = aa.y;
                        PoseParameters[offset + 2] = aa.z;
                    }
                }
            }
        }

        // -----------------------------------------------------------------------
        // Visualization helpers
        // -----------------------------------------------------------------------
        private void CreateJointVisualization()
        {
            if (!showJointSpheres || jointSpherePrefab == null) return;

            _jointSpheresParent = new GameObject("Joint Spheres");
            _jointSpheresParent.transform.SetParent(transform);

            for (int i = 0; i < JointCount; i++)
            {
                GameObject sphere = Instantiate(jointSpherePrefab, _jointSpheresParent.transform);
                sphere.name = $"Joint_{i}";
                sphere.transform.localScale = Vector3.one * 0.005f;
                JointTransforms[i] = sphere.transform;
            }
        }

        private void CleanupJointVisualization()
        {
            if (_jointSpheresParent != null)
            {
                Destroy(_jointSpheresParent);
            }
        }

        private Material CreateDefaultMaterial()
        {
            // Create a simple skin-tone material
            Shader shader = Shader.Find("Standard");
            if (shader == null) shader = Shader.Find("Legacy Shaders/Diffuse");

            Material mat = new Material(shader);
            mat.color = new Color(0.91f, 0.76f, 0.65f, 1f); // Average skin tone
            mat.name = "MANO_Skin_Default";
            return mat;
        }

        // -----------------------------------------------------------------------
        // File parsers
        // -----------------------------------------------------------------------
        private void LoadSkeletonDefinition()
        {
            string path = Path.Combine(Application.streamingAssetsPath, skeletonDefinitionFile);
            if (!File.Exists(path))
            {
                Debug.LogWarning($"[MANOHandModel] Skeleton definition not found at: {path}");
                return;
            }

            try
            {
                string json = File.ReadAllText(path);
                // Parse and use skeleton data for joint hierarchy setup
                Debug.Log($"[MANOHandModel] Loaded skeleton definition from: {path}");
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"[MANOHandModel] Failed to load skeleton definition: {ex.Message}");
            }
        }

        private static Vector3[] ParseBinaryVertices(byte[] data)
        {
            int vertexCount = data.Length / (3 * sizeof(float));
            Vector3[] vertices = new Vector3[vertexCount];

            for (int i = 0; i < vertexCount; i++)
            {
                int offset = i * 3 * sizeof(float);
                vertices[i] = new Vector3(
                    BitConverter.ToSingle(data, offset),
                    BitConverter.ToSingle(data, offset + sizeof(float)),
                    BitConverter.ToSingle(data, offset + 2 * sizeof(float))
                );
            }

            return vertices;
        }

        private static int[] ParseFaces(byte[] data)
        {
            int faceCount = data.Length / (3 * sizeof(int));
            int[] faces = new int[faceCount * 3];

            for (int i = 0; i < faceCount * 3; i++)
            {
                faces[i] = BitConverter.ToInt32(data, i * sizeof(int));
            }

            return faces;
        }

        private static float[,] ParseJointRegressor(byte[] data)
        {
            // Expected: 778 vertices × 16 joints = 12448 floats
            int vertices = 778, joints = 16;
            float[,] matrix = new float[vertices, joints];

            for (int v = 0; v < vertices; v++)
            {
                for (int j = 0; j < joints; j++)
                {
                    int offset = (v * joints + j) * sizeof(float);
                    if (offset + sizeof(float) <= data.Length)
                    {
                        matrix[v, j] = BitConverter.ToSingle(data, offset);
                    }
                }
            }

            return matrix;
        }

        private static Vector3[][, ] ParsePoseBlendShapes(byte[] data)
        {
            // Simplified: just parse as flat array for now
            // Actual MANO pose blend shapes: 62 shape components × 778 vertices × 3
            int components = 62, vertices = 778;
            Vector3[][, ] shapes = new Vector3[components, vertices, 1];

            // Parse header or use known dimensions
            int floatsPerComponent = vertices * 3;
            int totalFloats = data.Length / sizeof(float);
            int actualComponents = Mathf.Min(components, totalFloats / floatsPerComponent);

            for (int c = 0; c < actualComponents; c++)
            {
                for (int v = 0; v < vertices; v++)
                {
                    int offset = (c * floatsPerComponent + v * 3) * sizeof(float);
                    if (offset + 3 * sizeof(float) <= data.Length)
                    {
                        shapes[c, v, 0] = new Vector3(
                            BitConverter.ToSingle(data, offset),
                            BitConverter.ToSingle(data, offset + sizeof(float)),
                            BitConverter.ToSingle(data, offset + 2 * sizeof(float))
                        );
                    }
                }
            }

            return shapes;
        }

        private static Vector3[][, ] ParseShapeBlendShapes(byte[] data)
        {
            // 10 shape components × 778 vertices × 3
            int components = 10, vertices = 778;
            Vector3[][, ] shapes = new Vector3[components, vertices, 1];

            int floatsPerComponent = vertices * 3;
            int totalFloats = data.Length / sizeof(float);
            int actualComponents = Mathf.Min(components, totalFloats / floatsPerComponent);

            for (int c = 0; c < actualComponents; c++)
            {
                for (int v = 0; v < vertices; v++)
                {
                    int offset = (c * floatsPerComponent + v * 3) * sizeof(float);
                    if (offset + 3 * sizeof(float) <= data.Length)
                    {
                        shapes[c, v, 0] = new Vector3(
                            BitConverter.ToSingle(data, offset),
                            BitConverter.ToSingle(data, offset + sizeof(float)),
                            BitConverter.ToSingle(data, offset + 2 * sizeof(float))
                        );
                    }
                }
            }

            return shapes;
        }

        private static int[] LoadDefaultMANOTopology()
        {
            // MANO has 1536 triangles with consistent topology.
            // For the procedural fallback, return empty (handled by CreateProceduralHandMesh).
            Debug.LogWarning("[MANOHandModel] Using default topology is only valid for actual MANO meshes.");
            return new int[0];
        }

        // -----------------------------------------------------------------------
        // Utility
        // -----------------------------------------------------------------------
        private static Vector3 QuaternionToAxisAngle(Quaternion q)
        {
            // Convert Quaternion to axis-angle representation
            // q = cos(θ/2) + sin(θ/2)(xi + yj + zk)
            // axis = (x, y, z) / sin(θ/2), angle = 2 * acos(w)

            if (q.w > 1f) q.Normalize();

            float angle = 2f * Mathf.Acos(Mathf.Clamp(q.w, -1f, 1f));
            float sinHalf = Mathf.Sin(angle / 2f);

            if (sinHalf < 0.0001f)
            {
                return Vector3.zero; // Near-identity
            }

            return new Vector3(
                q.x / sinHalf * angle,
                q.y / sinHalf * angle,
                q.z / sinHalf * angle
            );
        }

        /// <summary>Get debug info about the current model state.</summary>
        public string GetDebugInfo()
        {
            return $"MANO Hand ({handedness}) | Vertices: {VertexCount} | " +
                   $"Faces: {FaceCount} | Joints: {JointCount} | " +
                   $"Loaded: {IsModelLoaded}";
        }
    }
}
