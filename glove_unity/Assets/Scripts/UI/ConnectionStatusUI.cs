// =============================================================================
// ConnectionStatusUI.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// UI component showing the current networking connection status with a colored
// indicator dot, latency display, and a manual reconnect button.
//
// Status colors:
//   Green  = Connected
//   Yellow = Connecting / Reconnecting
//   Red    = Disconnected / Error
//
// Layout: Canvas → StatusBar (HorizontalLayoutGroup)
//   - StatusDot (Image)
//   - StatusText (TMP_Text)
//   - LatencyText (TMP_Text)
//   - ReconnectButton
// =============================================================================

using System;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace EdgeAI.DataGlove.UI
{
    /// <summary>
    /// Displays the WebSocket/UDP connection status with visual indicators
    /// and provides a manual reconnect button.
    /// </summary>
    public class ConnectionStatusUI : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("Network Receiver References")]
        [SerializeField] private MonoBehaviour webSocketReceiver;
        [SerializeField] private MonoBehaviour udpReceiver;

        [Header("Status Indicator")]
        [Tooltip("Status dot (Image component with a circular sprite)")]
        [SerializeField] private Image statusDot;

        [Tooltip("Status description text")]
        [SerializeField] private TMP_Text statusText;

        [Tooltip("Connection detail text (URL, port, etc.)")]
        [SerializeField] private TMP_Text detailText;

        [Header("Latency Display")]
        [Tooltip("Latency text (shows round-trip time in ms)")]
        [SerializeField] private TMP_Text latencyText;

        [Tooltip("Latency warning threshold (ms) — turns yellow above this")]
        [SerializeField] private float latencyWarningThreshold = 50f;

        [Tooltip("Latency critical threshold (ms) — turns red above this")]
        [SerializeField] private float latencyCriticalThreshold = 100f;

        [Header("Statistics")]
        [Tooltip("Data rate display (messages per second)")]
        [SerializeField] private TMP_Text dataRateText;

        [Tooltip("Packet count display")]
        [SerializeField] private TMP_Text packetCountText;

        [Header("Controls")]
        [Tooltip("Reconnect button")]
        [SerializeField] private Button reconnectButton;

        [Tooltip("Auto-reconnect toggle")]
        [SerializeField] private Toggle autoReconnectToggle;

        [Header("Status Colors")]
        [SerializeField] private Color connectedColor = new Color(0.2f, 0.85f, 0.3f, 1f);
        [SerializeField] private Color connectingColor = new Color(1f, 0.8f, 0.1f, 1f);
        [SerializeField] private Color reconnectingColor = new Color(1f, 0.5f, 0.1f, 1f);
        [SerializeField] private Color disconnectedColor = new Color(0.6f, 0.6f, 0.6f, 1f);
        [SerializeField] private Color errorColor = new Color(0.9f, 0.15f, 0.15f, 1f);

        [Header("Animation")]
        [Tooltip("Enable blinking animation for connecting/reconnecting states")]
        [SerializeField] private bool enableBlinking = true;

        [Tooltip("Blink speed (blinks per second)")]
        [SerializeField] private float blinkSpeed = 2f;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public ConnectionStatus CurrentStatus { get; private set; } = ConnectionStatus.Disconnected;
        public float CurrentLatencyMs { get; private set; }
        public int CurrentDataRate { get; private set; }

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private Networking.WebSocketReceiver _wsReceiver;
        private Networking.UDPReceiver _udpReceiver;
        private bool _isUsingWebSocket = true; // Default to WebSocket
        private float _blinkTimer;
        private bool _blinkState = true;
        private long _totalPackets;
        private DateTime _sessionStartTime;

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            ValidateReferences();
            sessionStartTime = DateTime.UtcNow;

            if (reconnectButton != null)
            {
                reconnectButton.onClick.AddListener(OnReconnectClicked);
            }
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
            // Auto-detect receivers if not assigned
            if (webSocketReceiver == null)
                webSocketReceiver = FindAnyObjectByType<Networking.WebSocketReceiver>();
            if (udpReceiver == null)
                udpReceiver = FindAnyObjectByType<Networking.UDPReceiver>();

            DetermineActiveReceiver();
            SetStatus(ConnectionStatus.Disconnected, "Waiting for connection");
        }

        private void Update()
        {
            UpdateLatencyDisplay();
            UpdateDataRateDisplay();
            UpdateBlinking();
        }

        // -----------------------------------------------------------------------
        // Event subscription
        // -----------------------------------------------------------------------
        private void SubscribeToEvents()
        {
            if (_wsReceiver != null)
            {
                _wsReceiver.OnConnectionStatusChanged += HandleConnectionStatusChanged;
            }
            if (_udpReceiver != null)
            {
                _udpReceiver.OnConnectionStatusChanged += HandleConnectionStatusChanged;
            }
        }

        private void UnsubscribeFromEvents()
        {
            if (_wsReceiver != null)
            {
                _wsReceiver.OnConnectionStatusChanged -= HandleConnectionStatusChanged;
            }
            if (_udpReceiver != null)
            {
                _udpReceiver.OnConnectionStatusChanged -= HandleConnectionStatusChanged;
            }
        }

        // -----------------------------------------------------------------------
        // Status update
        // -----------------------------------------------------------------------
        private void HandleConnectionStatusChanged(ConnectionStatus status, string message)
        {
            CurrentStatus = status;
            SetStatus(status, message);
        }

        private void SetStatus(ConnectionStatus status, string message)
        {
            CurrentStatus = status;

            // Update status dot color
            if (statusDot != null)
            {
                statusDot.color = GetStatusColor(status);
            }

            // Update status text
            if (statusText != null)
            {
                statusText.text = GetStatusText(status);
            }

            // Update detail text
            if (detailText != null)
            {
                string transportType = _isUsingWebSocket ? "WebSocket" : "UDP";
                string connectionInfo = _isUsingWebSocket
                    ? (_wsReceiver != null ? $"ws://localhost:8765" : "")
                    : (_udpReceiver != null ? $"UDP :9999" : "");
                detailText.text = $"{transportType} | {connectionInfo}";
            }
        }

        // -----------------------------------------------------------------------
        // Latency and data rate
        // -----------------------------------------------------------------------
        private void UpdateLatencyDisplay()
        {
            float latency = 0f;
            int dataRate = 0;

            if (_isUsingWebSocket && _wsReceiver != null)
            {
                latency = _wsReceiver.CurrentLatencyMs;
                dataRate = _wsReceiver.MessagesPerSecond;
            }
            else if (!_isUsingWebSocket && _udpReceiver != null)
            {
                latency = _udpReceiver.CurrentLatencyMs;
                dataRate = _udpReceiver.PacketsPerSecond;
            }

            CurrentLatencyMs = latency;
            CurrentDataRate = dataRate;

            if (latencyText != null)
            {
                if (CurrentStatus == ConnectionStatus.Connected)
                {
                    latencyText.text = $"{latency:F0} ms";
                    latencyText.color = GetLatencyColor(latency);
                }
                else
                {
                    latencyText.text = "— ms";
                    latencyText.color = Color.gray;
                }
            }
        }

        private void UpdateDataRateDisplay()
        {
            if (dataRateText != null && CurrentStatus == ConnectionStatus.Connected)
            {
                dataRateText.text = $"{CurrentDataRate} msg/s";
            }
            else if (dataRateText != null)
            {
                dataRateText.text = "— msg/s";
            }
        }

        // -----------------------------------------------------------------------
        // Blinking animation
        // -----------------------------------------------------------------------
        private void UpdateBlinking()
        {
            if (!enableBlinking || statusDot == null) return;

            bool shouldBlink = CurrentStatus == ConnectionStatus.Connecting
                            || CurrentStatus == ConnectionStatus.Reconnecting;

            if (shouldBlink)
            {
                _blinkTimer += Time.deltaTime * blinkSpeed;
                _blinkState = Mathf.Sin(_blinkTimer * Mathf.PI * 2f) > 0f;

                Color baseColor = GetStatusColor(CurrentStatus);
                statusDot.color = _blinkState ? baseColor : new Color(baseColor.r, baseColor.g, baseColor.b, 0.2f);
            }
        }

        // -----------------------------------------------------------------------
        // User interactions
        // -----------------------------------------------------------------------
        private void OnReconnectClicked()
        {
            if (_isUsingWebSocket && _wsReceiver != null)
            {
                _wsReceiver.Reconnect();
            }
            else if (!_isUsingWebSocket && _udpReceiver != null)
            {
                _udpReceiver.StopListening();
                _udpReceiver.StartListening();
            }
        }

        /// <summary>Toggle between WebSocket and UDP transport.</summary>
        public void SwitchTransport(bool useWebSocket)
        {
            _isUsingWebSocket = useWebSocket;

            // Disconnect current
            if (_wsReceiver != null && !useWebSocket) _wsReceiver.Disconnect();
            if (_udpReceiver != null && useWebSocket) _udpReceiver.StopListening();

            // Connect new
            if (useWebSocket && _wsReceiver != null) _wsReceiver.Connect();
            if (!useWebSocket && _udpReceiver != null) _udpReceiver.StartListening();

            DetermineActiveReceiver();
        }

        // -----------------------------------------------------------------------
        // Helpers
        // -----------------------------------------------------------------------
        private void DetermineActiveReceiver()
        {
            _isUsingWebSocket = _wsReceiver != null && _udpReceiver == null;
        }

        private void ValidateReferences()
        {
            if (webSocketReceiver != null && !(webSocketReceiver is Networking.WebSocketReceiver))
            {
                Debug.LogError("[ConnectionStatusUI] webSocketReceiver must be of type WebSocketReceiver.");
                webSocketReceiver = null;
            }

            if (udpReceiver != null && !(udpReceiver is Networking.UDPReceiver))
            {
                Debug.LogError("[ConnectionStatusUI] udpReceiver must be of type UDPReceiver.");
                udpReceiver = null;
            }

            // Cache typed references
            _wsReceiver = webSocketReceiver as Networking.WebSocketReceiver;
            _udpReceiver = udpReceiver as Networking.UDPReceiver;
        }

        private Color GetStatusColor(ConnectionStatus status)
        {
            switch (status)
            {
                case ConnectionStatus.Connected:    return connectedColor;
                case ConnectionStatus.Connecting:   return connectingColor;
                case ConnectionStatus.Reconnecting: return reconnectingColor;
                case ConnectionStatus.Disconnected: return disconnectedColor;
                case ConnectionStatus.Error:        return errorColor;
                default:                            return disconnectedColor;
            }
        }

        private string GetStatusText(ConnectionStatus status)
        {
            switch (status)
            {
                case ConnectionStatus.Connected:    return "● Connected";
                case ConnectionStatus.Connecting:   return "◐ Connecting...";
                case ConnectionStatus.Reconnecting: return "◑ Reconnecting...";
                case ConnectionStatus.Disconnected: return "○ Disconnected";
                case ConnectionStatus.Error:        return "✕ Error";
                default:                            return "? Unknown";
            }
        }

        private Color GetLatencyColor(float latencyMs)
        {
            if (latencyMs <= latencyWarningThreshold) return connectedColor;
            if (latencyMs <= latencyCriticalThreshold) return connectingColor;
            return errorColor;
        }
    }
}
