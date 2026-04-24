// =============================================================================
// GestureDisplay.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// UI component for displaying the currently recognized gesture, confidence
// score, and NLP-corrected text output.
//
// Requires: Unity UI (UnityEngine.UI) package
// Layout: Canvas → GestureDisplayPanel (VerticalLayoutGroup)
//   - GestureNameText
//   - ConfidenceBar (Image fill)
//   - ConfidenceText
//   - NLPOutputText
//   - GestureHistory (ScrollRect with reusable labels)
// =============================================================================

using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace EdgeAI.DataGlove.UI
{
    /// <summary>
    /// Displays gesture recognition results in the Unity UI.
    /// Subscribes to HandController events and renders the current gesture,
    /// confidence level, NLP-corrected text, and a scrolling history log.
    /// </summary>
    public class GestureDisplay : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields — UI References
        // -----------------------------------------------------------------------
        [Header("Hand Controller Reference")]
        [SerializeField] private HandController handController;

        [Header("Gesture Display")]
        [Tooltip("Large text showing the current recognized gesture (Chinese characters)")]
        [SerializeField] private TMP_Text gestureNameText;

        [Tooltip("Pinyin or English translation of the gesture")]
        [SerializeField] private TMP_Text gestureTranslationText;

        [Header("Confidence Display")]
        [Tooltip("Confidence bar (Image with Fill type)")]
        [SerializeField] private Image confidenceBar;

        [Tooltip("Confidence percentage text")]
        [SerializeField] private TMP_Text confidenceText;

        [Tooltip("Low confidence threshold — bar turns yellow below this")]
        [SerializeField] [Range(0f, 1f)] private float lowConfidenceThreshold = 0.5f;

        [Tooltip("High confidence threshold — bar turns green above this")]
        [SerializeField] [Range(0f, 1f)] private float highConfidenceThreshold = 0.8f;

        [Tooltip("Colors for confidence levels")]
        [SerializeField] private Color highConfidenceColor = new Color(0.2f, 0.8f, 0.3f, 1f);
        [SerializeField] private Color mediumConfidenceColor = new Color(0.9f, 0.7f, 0.1f, 1f);
        [SerializeField] private Color lowConfidenceColor = new Color(0.85f, 0.2f, 0.2f, 1f);

        [Header("NLP Output")]
        [Tooltip("NLP post-processed text output")]
        [SerializeField] private TMP_Text nlpOutputText;

        [Tooltip("NLP output panel (shown/hidden based on content)")]
        [SerializeField] private GameObject nlpOutputPanel;

        [Header("Gesture History")]
        [Tooltip("History log scroll area")]
        [SerializeField] private ScrollRect historyScrollRect;

        [Tooltip("History content parent (where log entries are instantiated)")]
        [SerializeField] private Transform historyContentParent;

        [Tooltip("Prefab for a single history entry")]
        [SerializeField] private GameObject historyEntryPrefab;

        [Tooltip("Maximum history entries to keep")]
        [SerializeField] private int maxHistoryEntries = 50;

        [Header("Timing")]
        [Tooltip("Minimum time between same-gesture updates (debounce, seconds)")]
        [SerializeField] private float gestureDebounceTime = 0.5f;

        [Tooltip("Animation duration for gesture transition")]
        [SerializeField] private float transitionAnimDuration = 0.2f;

        // -----------------------------------------------------------------------
        // Public events
        // -----------------------------------------------------------------------
        /// <summary>Fired when a new gesture is displayed.</summary>
        public event Action<string, float> OnGestureDisplayed;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public string CurrentGestureLabel { get; private set; } = "";
        public float CurrentConfidence { get; private set; }
        public string CurrentNLPText { get; private set; } = "";
        public int HistoryCount => _gestureHistory.Count;

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private List<GestureResult> _gestureHistory = new List<GestureResult>();
        private string _lastGestureLabel = "";
        private float _lastGestureTime;
        private bool _isAnimating;

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            ValidateUIReferences();
            SetDefaultState();
        }

        private void OnEnable()
        {
            SubscribeToEvents();
        }

        private void OnDisable()
        {
            UnsubscribeFromEvents();
        }

        private void Start()
        {
            // Auto-find HandController if not assigned
            if (handController == null)
            {
                handController = FindAnyObjectByType<HandController>();
                if (handController == null)
                {
                    Debug.LogWarning("[GestureDisplay] No HandController found in scene. " +
                                     "Gesture updates will not be received.");
                }
            }
        }

        // -----------------------------------------------------------------------
        // Event subscription
        // -----------------------------------------------------------------------
        private void SubscribeToEvents()
        {
            if (handController != null)
            {
                handController.OnGestureRecognized += HandleGestureRecognized;
            }
        }

        private void UnsubscribeFromEvents()
        {
            if (handController != null)
            {
                handController.OnGestureRecognized -= HandleGestureRecognized;
            }
        }

        /// <summary>Re-subscribe to a different HandController at runtime.</summary>
        public void SetHandController(HandController controller)
        {
            UnsubscribeFromEvents();
            handController = controller;
            SubscribeToEvents();
        }

        // -----------------------------------------------------------------------
        // Gesture handling
        // -----------------------------------------------------------------------
        private void HandleGestureRecognized(GestureResult result)
        {
            if (result.label == _lastGestureLabel
                && Time.time - _lastGestureTime < gestureDebounceTime)
            {
                // Debounce: skip if same gesture within debounce window
                UpdateConfidence(result.confidence);
                return;
            }

            _lastGestureLabel = result.label;
            _lastGestureTime = Time.time;

            // Update display
            UpdateGestureName(result.label);
            UpdateConfidence(result.confidence);
            UpdateNLPOutput(result.nlpText);
            AddToHistory(result);

            OnGestureDisplayed?.Invoke(result.label, result.confidence);
        }

        // -----------------------------------------------------------------------
        // UI update methods
        // -----------------------------------------------------------------------

        /// <summary>Update the gesture name text with animation.</summary>
        private void UpdateGestureName(string gesture)
        {
            CurrentGestureLabel = gesture;

            if (gestureNameText != null)
            {
                gestureNameText.text = string.IsNullOrEmpty(gesture) ? "—" : gesture;

                // Scale animation
                if (!_isAnimating)
                {
                    StartCoroutine(ScaleAnimation(gestureNameText.transform, transitionAnimDuration));
                }
            }

            // Try to show translation (optional enhancement)
            if (gestureTranslationText != null)
            {
                string translation = GetTranslation(gesture);
                gestureTranslationText.text = string.IsNullOrEmpty(translation) ? "" : $"({translation})";
            }
        }

        /// <summary>Update the confidence bar and text.</summary>
        private void UpdateConfidence(float confidence)
        {
            CurrentConfidence = confidence;

            if (confidenceBar != null)
            {
                confidenceBar.fillAmount = confidence;
                confidenceBar.color = GetConfidenceColor(confidence);
            }

            if (confidenceText != null)
            {
                confidenceText.text = $"{(confidence * 100f):F0}%";
            }
        }

        /// <summary>Update the NLP output text.</summary>
        private void UpdateNLPOutput(string nlpText)
        {
            CurrentNLPText = nlpText;

            if (nlpOutputText != null)
            {
                nlpOutputText.text = string.IsNullOrEmpty(nlpText) ? "" : nlpText;
            }

            if (nlpOutputPanel != null)
            {
                nlpOutputPanel.SetActive(!string.IsNullOrEmpty(nlpText));
            }
        }

        /// <summary>Add a gesture result to the history log.</summary>
        private void AddToHistory(GestureResult result)
        {
            _gestureHistory.Add(result);

            // Trim excess entries
            while (_gestureHistory.Count > maxHistoryEntries)
            {
                _gestureHistory.RemoveAt(0);
            }

            // Create visual entry
            CreateHistoryEntry(result);
        }

        private void CreateHistoryEntry(GestureResult result)
        {
            if (historyContentParent == null || historyEntryPrefab == null) return;

            GameObject entry = Instantiate(historyEntryPrefab, historyContentParent);
            entry.name = $"HistoryEntry_{_gestureHistory.Count}";

            // Find text components in the prefab
            TMP_Text[] texts = entry.GetComponentsInChildren<TMP_Text>();
            if (texts.Length >= 2)
            {
                texts[0].text = result.label;
                texts[1].text = $"{(result.confidence * 100f):F0}%";
                texts[1].color = GetConfidenceColor(result.confidence);
            }
            else if (texts.Length >= 1)
            {
                texts[0].text = $"{result.label} ({(result.confidence * 100f):F0}%)";
            }

            // Auto-scroll to bottom
            if (historyScrollRect != null)
            {
                Canvas.ForceUpdateCanvases();
                historyScrollRect.verticalNormalizedPosition = 0f;
            }
        }

        // -----------------------------------------------------------------------
        // Helpers
        // -----------------------------------------------------------------------
        private Color GetConfidenceColor(float confidence)
        {
            if (confidence >= highConfidenceThreshold) return highConfidenceColor;
            if (confidence >= lowConfidenceThreshold) return mediumConfidenceColor;
            return lowConfidenceColor;
        }

        private void SetDefaultState()
        {
            if (gestureNameText != null) gestureNameText.text = "—";
            if (gestureTranslationText != null) gestureTranslationText.text = "";
            if (confidenceText != null) confidenceText.text = "0%";
            if (confidenceBar != null) confidenceBar.fillAmount = 0f;
            if (nlpOutputText != null) nlpOutputText.text = "";
            if (nlpOutputPanel != null) nlpOutputPanel.SetActive(false);
        }

        private void ValidateUIReferences()
        {
            if (gestureNameText == null)
                Debug.LogWarning("[GestureDisplay] gestureNameText not assigned.");
            if (confidenceBar == null)
                Debug.LogWarning("[GestureDisplay] confidenceBar not assigned.");
        }

        /// <summary>
        /// Simple gesture-to-translation lookup (expandable).
        /// In production, load from gesture_labels.json or a localization system.
        /// </summary>
        private string GetTranslation(string gesture)
        {
            // Common CSL gestures — extend as needed
            var translations = new Dictionary<string, string>
            {
                { "你好",     "Hello" },
                { "谢谢",     "Thank you" },
                { "对不起",   "Sorry" },
                { "再见",     "Goodbye" },
                { "爱",       "Love" },
                { "家",       "Home" },
                { "朋友",     "Friend" },
                { "学习",     "Study" },
                { "工作",     "Work" },
                { "吃饭",     "Eat" },
            };

            return translations.TryGetValue(gesture, out string t) ? t : "";
        }

        // -----------------------------------------------------------------------
        // Animation
        // -----------------------------------------------------------------------
        private System.Collections.IEnumerator ScaleAnimation(Transform target, float duration)
        {
            _isAnimating = true;
            Vector3 originalScale = target.localScale;
            Vector3 peakScale = originalScale * 1.15f;

            // Scale up
            float elapsed = 0f;
            while (elapsed < duration / 2f)
            {
                float t = elapsed / (duration / 2f);
                target.localScale = Vector3.Lerp(originalScale, peakScale, t);
                elapsed += Time.deltaTime;
                yield return null;
            }

            // Scale back down
            elapsed = 0f;
            while (elapsed < duration / 2f)
            {
                float t = elapsed / (duration / 2f);
                target.localScale = Vector3.Lerp(peakScale, originalScale, t);
                elapsed += Time.deltaTime;
                yield return null;
            }

            target.localScale = originalScale;
            _isAnimating = false;
        }

        // -----------------------------------------------------------------------
        // Public API for external control
        // -----------------------------------------------------------------------

        /// <summary>Manually set the gesture display (e.g., for demo mode).</summary>
        public void SetGesture(string label, float confidence, string nlpText = "")
        {
            var result = new GestureResult(label, confidence, nlpText, DateTimeOffset.UtcNow.ToUnixTimeSeconds());
            HandleGestureRecognized(result);
        }

        /// <summary>Clear all history entries.</summary>
        public void ClearHistory()
        {
            _gestureHistory.Clear();
            if (historyContentParent != null)
            {
                foreach (Transform child in historyContentParent)
                {
                    Destroy(child.gameObject);
                }
            }
        }

        /// <summary>Get the full gesture history for export/logging.</summary>
        public List<GestureResult> GetHistory() => new List<GestureResult>(_gestureHistory);
    }
}
