// =============================================================================
// WebSocketReceiver.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// WebSocket client that connects to the Python Relay server (ws://localhost:8765),
// receives JSON-encoded SensorMessage payloads, and dispatches them on the
// main Unity thread via a thread-safe queue.
//
// Dependencies:
//   - NativeWebSocket (https://github.com/endel/NativeWebSocket)
//     or System.Net.WebSockets (Unity 2022+ via .NET Standard 2.1)
// =============================================================================

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using UnityEngine;

namespace EdgeAI.DataGlove.Networking
{
    /// <summary>
    /// WebSocket receiver that connects to the Python Relay server and feeds
    /// SensorMessage data to the HandController.
    /// </summary>
    public class WebSocketReceiver : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("Connection Settings")]
        [Tooltip("WebSocket server URL (Python Relay)")]
        [SerializeField] private string serverUrl = "ws://localhost:8765";

        [Tooltip("Connection timeout in seconds")]
        [SerializeField] private float connectionTimeout = 5f;

        [Header("Reconnection Settings")]
        [Tooltip("Enable automatic reconnection on disconnect")]
        [SerializeField] private bool autoReconnect = true;

        [Tooltip("Delay between reconnection attempts (seconds)")]
        [SerializeField] private float reconnectDelay = 2f;

        [Tooltip("Maximum number of reconnection attempts (0 = infinite)")]
        [SerializeField] private int maxReconnectAttempts = 0;

        [Header("Performance")]
        [Tooltip("Maximum messages to process per frame")]
        [SerializeField] private int maxMessagesPerFrame = 5;

        // -----------------------------------------------------------------------
        // Public events
        // -----------------------------------------------------------------------
        /// <summary>Fired when a complete SensorMessage is received and parsed.</summary>
        public event Action<SensorMessage> OnMessageReceived;

        /// <summary>Fired when connection state changes.</summary>
        public event Action<ConnectionStatus, string> OnConnectionStatusChanged;

        /// <summary>Fired on any error (parse error, network error, etc.).</summary>
        public event Action<string> OnError;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public ConnectionStatus Status { get; private set; } = ConnectionStatus.Disconnected;
        public bool IsConnected => Status == ConnectionStatus.Connected;
        public float CurrentLatencyMs { get; private set; }
        public int MessagesPerSecond { get; private set; }

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private ThreadSafeQueue<string> _messageQueue = new ThreadSafeQueue<string>();
        private Thread _receiveThread;
        private volatile bool _isRunning;
        private int _reconnectAttempts;
        private float _lastMessageTime;
        private int _messageCount;
        private float _mpsAccumulator;
        private float _mpsInterval = 1f;

#if UNITY_WEBGL && !UNITY_EDITOR
        // WebGL uses JavaScript-based WebSocket
        private object _webSocket;
#else
        // Standalone / Editor uses System.Net.WebSockets
        private System.Net.WebSockets.ClientWebSocket _webSocket;
#endif

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            DontDestroyOnLoad(this.gameObject);
        }

        private void Update()
        {
            // Process queued messages on the main thread
            ProcessMessageQueue();

            // Track messages per second
            _mpsAccumulator += Time.deltaTime;
            if (_mpsAccumulator >= _mpsInterval)
            {
                MessagesPerSecond = _messageCount;
                _messageCount = 0;
                _mpsAccumulator -= _mpsInterval;
            }
        }

        private void OnDestroy()
        {
            Disconnect();
        }

        private void OnApplicationPause(bool pauseStatus)
        {
            if (pauseStatus && IsConnected)
            {
                // Don't disconnect on pause — keep receiving in background thread
                Debug.Log("[WebSocketReceiver] Application paused, connection maintained.");
            }
        }

        private void OnApplicationQuit()
        {
            Disconnect();
        }

        // -----------------------------------------------------------------------
        // Public methods
        // -----------------------------------------------------------------------

        /// <summary>Start connecting to the WebSocket server (non-blocking).</summary>
        public void Connect()
        {
            if (_isRunning)
            {
                Debug.LogWarning("[WebSocketReceiver] Already running.");
                return;
            }

            SetStatus(ConnectionStatus.Connecting, "Connecting to " + serverUrl);
            _isRunning = true;
            _reconnectAttempts = 0;
            _receiveThread = new Thread(ReceiveLoop)
            {
                Name = "WebSocketReceiver",
                IsBackground = true
            };
            _receiveThread.Start();
        }

        /// <summary>Disconnect from the server and stop the receive thread.</summary>
        public void Disconnect()
        {
            _isRunning = false;
            SetStatus(ConnectionStatus.Disconnected, "Disconnected");

            try
            {
                CloseWebSocket();
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"[WebSocketReceiver] Close error: {ex.Message}");
            }

            if (_receiveThread != null && _receiveThread.IsAlive)
            {
                if (!_receiveThread.Join(2000))
                {
                    Debug.LogWarning("[WebSocketReceiver] Receive thread did not exit gracefully.");
                }
                _receiveThread = null;
            }
        }

        /// <summary>Manually trigger a reconnect.</summary>
        public void Reconnect()
        {
            Disconnect();
            Connect();
        }

        /// <summary>Change the server URL at runtime.</summary>
        public void SetServerUrl(string url)
        {
            if (IsConnected)
            {
                Debug.LogWarning("[WebSocketReceiver] Cannot change URL while connected. Disconnect first.");
                return;
            }
            serverUrl = url;
            Debug.Log($"[WebSocketReceiver] Server URL updated to: {url}");
        }

        // -----------------------------------------------------------------------
        // Connection management (private)
        // -----------------------------------------------------------------------
        private void ReceiveLoop()
        {
            while (_isRunning)
            {
                try
                {
                    ConnectWebSocket();

                    if (_webSocket == null)
                    {
                        HandleReconnect("WebSocket creation failed");
                        continue;
                    }

                    // Wait for connection
                    if (!WaitForConnection())
                    {
                        continue;
                    }

                    SetStatus(ConnectionStatus.Connected, "Connected");
                    _reconnectAttempts = 0;

                    // Main receive loop
                    var receiveBuffer = new byte[8192];
                    var segment = new ArraySegment<byte>(receiveBuffer);

                    while (_isRunning && IsConnected)
                    {
#if !UNITY_WEBGL || UNITY_EDITOR
                        if (_webSocket.State != System.Net.WebSockets.WebSocketState.Open)
                            break;
#endif

                        System.Net.WebSockets.WebSocketReceiveResult result = null;

#if !UNITY_WEBGL || UNITY_EDITOR
                        result = _webSocket.ReceiveAsync(segment, CancellationToken.None).GetAwaiter().GetResult();
#else
                        break; // WebGL handled differently
#endif

                        if (result == null) continue;

                        if (result.MessageType == System.Net.WebSockets.WebSocketMessageType.Close)
                        {
                            Debug.Log("[WebSocketReceiver] Server sent close frame.");
                            break;
                        }

                        if (result.MessageType == System.Net.WebSockets.WebSocketMessageType.Text)
                        {
                            string json = Encoding.UTF8.GetString(receiveBuffer, 0, result.Count);
                            _messageQueue.Enqueue(json);
                            _messageCount++;
                            _lastMessageTime = Time.realtimeSinceStartup;
                        }
                    }
                }
                catch (OperationCanceledException)
                {
                    // Expected on shutdown
                    break;
                }
                catch (System.Net.WebSockets.WebSocketException wex)
                {
                    Debug.LogWarning($"[WebSocketReceiver] WebSocket error: {wex.WebSocketErrorCode} — {wex.Message}");
                    HandleReconnect(wex.Message);
                }
                catch (Exception ex)
                {
                    Debug.LogError($"[WebSocketReceiver] Receive loop error: {ex.Message}");
                    HandleReconnect(ex.Message);
                }
                finally
                {
                    CloseWebSocket();
                    SetStatus(ConnectionStatus.Disconnected, "Connection lost");
                }
            }
        }

        private void ConnectWebSocket()
        {
#if !UNITY_WEBGL || UNITY_EDITOR
            _webSocket = new System.Net.WebSockets.ClientWebSocket();
            _webSocket.Options.KeepAliveInterval = TimeSpan.FromSeconds(5);
            var connectTask = _webSocket.ConnectAsync(
                new Uri(serverUrl),
                CancellationToken.None
            );
            connectTask.GetAwaiter().GetResult();
#endif
        }

        private bool WaitForConnection()
        {
            float elapsed = 0f;
            while (elapsed < connectionTimeout)
            {
                if (!_isRunning) return false;

#if !UNITY_WEBGL || UNITY_EDITOR
                if (_webSocket.State == System.Net.WebSockets.WebSocketState.Open)
                    return true;

                if (_webSocket.State == System.Net.WebSockets.WebSocketState.Aborted ||
                    _webSocket.State == System.Net.WebSockets.WebSocketState.Closed)
                    return false;
#endif

                Thread.Sleep(100);
                elapsed += 0.1f;
            }

            Debug.LogWarning("[WebSocketReceiver] Connection timed out.");
            return false;
        }

        private void CloseWebSocket()
        {
#if !UNITY_WEBGL || UNITY_EDITOR
            if (_webSocket != null)
            {
                try
                {
                    if (_webSocket.State == System.Net.WebSockets.WebSocketState.Open)
                    {
                        _webSocket.CloseAsync(
                            System.Net.WebSockets.WebSocketCloseStatus.NormalClosure,
                            "Client closing",
                            CancellationToken.None
                        ).GetAwaiter().GetResult();
                    }
                }
                catch { /* swallow close errors */ }
                finally
                {
                    _webSocket.Dispose();
                    _webSocket = null;
                }
            }
#endif
        }

        private void HandleReconnect(string reason)
        {
            if (!autoReconnect || !_isRunning)
            {
                SetStatus(ConnectionStatus.Disconnected, reason);
                return;
            }

            if (maxReconnectAttempts > 0 && _reconnectAttempts >= maxReconnectAttempts)
            {
                SetStatus(ConnectionStatus.Error, $"Max reconnect attempts ({maxReconnectAttempts}) reached.");
                _isRunning = false;
                return;
            }

            _reconnectAttempts++;
            SetStatus(ConnectionStatus.Reconnecting, $"Reconnecting ({_reconnectAttempts})... {reason}");

            // Wait before reconnecting
            float wait = reconnectDelay * Mathf.Min(_reconnectAttempts, 5f); // Exponential-ish backoff
            int sleepMs = Mathf.RoundToInt(wait * 1000f);
            for (int i = 0; i < sleepMs && _isRunning; i += 100)
            {
                Thread.Sleep(100);
            }
        }

        // -----------------------------------------------------------------------
        // Main-thread message processing
        // -----------------------------------------------------------------------
        private void ProcessMessageQueue()
        {
            int processed = 0;
            while (processed < maxMessagesPerFrame && _messageQueue.TryDequeue(out string json))
            {
                try
                {
                    SensorMessage msg = ParseJson(json);
                    if (msg != null && msg.IsValid())
                    {
                        // Calculate round-trip latency estimate
                        if (msg.timestamp > 0)
                        {
                            double now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                            CurrentLatencyMs = Mathf.Abs((float)((now - msg.timestamp) * 1000.0));
                        }

                        OnMessageReceived?.Invoke(msg);
                    }
                }
                catch (Exception ex)
                {
                    Debug.LogError($"[WebSocketReceiver] Parse error: {ex.Message}");
                    OnError?.Invoke($"Parse error: {ex.Message}");
                }

                processed++;
            }
        }

        // -----------------------------------------------------------------------
        // JSON parsing
        // -----------------------------------------------------------------------
        private SensorMessage ParseJson(string json)
        {
            // Unity 2022 includes System.Text.Json via .NET Standard 2.1,
            // but JsonUtility works for simple [Serializable] types.
            // For nested arrays (hall, imu_euler, keypoints), we use a
            // lightweight custom parser or JsonUtility with a wrapper.

            // Attempt JsonUtility first
            try
            {
                var wrapper = JsonUtility.FromJson<SensorMessageWrapper>(json);
                if (wrapper != null)
                {
                    return wrapper.ToSensorMessage();
                }
            }
            catch (Exception)
            {
                // Fall through to manual parsing
            }

            // Manual JSON parsing fallback for full flexibility
            return ManualParseSensorMessage(json);
        }

        /// <summary>
        /// Lightweight manual JSON parser for SensorMessage.
        /// Handles nested arrays that JsonUtility can't deserialize directly.
        /// </summary>
        private SensorMessage ManualParseSensorMessage(string json)
        {
            var msg = new SensorMessage();

            // Parse top-level simple fields
            msg.timestamp = ExtractDouble(json, "timestamp");
            msg.gesture = ExtractString(json, "gesture");
            msg.confidence = ExtractFloat(json, "confidence");
            msg.nlp_text = ExtractString(json, "nlp_text");

            // Parse arrays
            msg.hall = ExtractFloatArray(json, "hall");
            msg.imu_euler = ExtractFloatArray(json, "imu_euler");
            msg.imu_gyro = ExtractFloatArray(json, "imu_gyro");

            // Parse keypoints array of objects
            msg.keypoints = ExtractKeypointArray(json, "keypoints");

            return msg;
        }

        // -----------------------------------------------------------------------
        // Simple JSON extraction helpers (no external dependencies)
        // -----------------------------------------------------------------------
        private static float ExtractFloat(string json, string key)
        {
            string pattern = $"\"{key}\"";
            int idx = json.IndexOf(pattern, StringComparison.Ordinal);
            if (idx < 0) return 0f;

            int colonIdx = json.IndexOf(':', idx + pattern.Length);
            if (colonIdx < 0) return 0f;

            // Skip whitespace
            int start = colonIdx + 1;
            while (start < json.Length && char.IsWhiteSpace(json[start])) start++;

            // Read number
            int end = start;
            bool hasDecimal = false;
            bool hasSign = (end < json.Length && (json[end] == '-' || json[end] == '+'));

            if (hasSign) end++;
            while (end < json.Length && (char.IsDigit(json[end]) || json[end] == '.' || json[end] == 'e' || json[end] == 'E' || json[end] == '+' || json[end] == '-'))
            {
                if (json[end] == '.') hasDecimal = true;
                end++;
            }

            if (end > start)
            {
                string numStr = json.Substring(start, end - start);
                return float.Parse(numStr, System.Globalization.CultureInfo.InvariantCulture);
            }

            return 0f;
        }

        private static double ExtractDouble(string json, string key)
        {
            string pattern = $"\"{key}\"";
            int idx = json.IndexOf(pattern, StringComparison.Ordinal);
            if (idx < 0) return 0.0;

            int colonIdx = json.IndexOf(':', idx + pattern.Length);
            if (colonIdx < 0) return 0.0;

            int start = colonIdx + 1;
            while (start < json.Length && char.IsWhiteSpace(json[start])) start++;

            int end = start;
            while (end < json.Length && (char.IsDigit(json[end]) || json[end] == '.' || json[end] == 'e' || json[end] == 'E' || json[end] == '+' || json[end] == '-'))
            {
                end++;
            }

            if (end > start)
            {
                return double.Parse(json.Substring(start, end - start), System.Globalization.CultureInfo.InvariantCulture);
            }

            return 0.0;
        }

        private static string ExtractString(string json, string key)
        {
            string pattern = $"\"{key}\"";
            int idx = json.IndexOf(pattern, StringComparison.Ordinal);
            if (idx < 0) return null;

            int colonIdx = json.IndexOf(':', idx + pattern.Length);
            if (colonIdx < 0) return null;

            int quoteStart = json.IndexOf('"', colonIdx + 1);
            if (quoteStart < 0) return null;

            int quoteEnd = json.IndexOf('"', quoteStart + 1);
            if (quoteEnd < 0) return null;

            return json.Substring(quoteStart + 1, quoteEnd - quoteStart - 1);
        }

        private static float[] ExtractFloatArray(string json, string key)
        {
            string pattern = $"\"{key}\"";
            int idx = json.IndexOf(pattern, StringComparison.Ordinal);
            if (idx < 0) return new float[0];

            int bracketStart = json.IndexOf('[', idx + pattern.Length);
            if (bracketStart < 0) return new float[0];

            int bracketEnd = json.IndexOf(']', bracketStart + 1);
            if (bracketEnd < 0) return new float[0];

            string inner = json.Substring(bracketStart + 1, bracketEnd - bracketStart - 1).Trim();
            if (string.IsNullOrEmpty(inner)) return new float[0];

            string[] parts = inner.Split(',');
            float[] result = new float[parts.Length];
            for (int i = 0; i < parts.Length; i++)
            {
                if (float.TryParse(parts[i].Trim(), System.Globalization.NumberStyles.Float,
                    System.Globalization.CultureInfo.InvariantCulture, out float val))
                {
                    result[i] = val;
                }
            }

            return result;
        }

        private static Keypoint3D[] ExtractKeypointArray(string json, string key)
        {
            // Find the outer array of keypoint objects
            string pattern = $"\"{key}\"";
            int idx = json.IndexOf(pattern, StringComparison.Ordinal);
            if (idx < 0) return null;

            int bracketStart = json.IndexOf('[', idx + pattern.Length);
            if (bracketStart < 0) return null;

            // Find matching bracket — count depth
            int depth = 0;
            int bracketEnd = -1;
            for (int i = bracketStart; i < json.Length; i++)
            {
                if (json[i] == '[') depth++;
                else if (json[i] == ']')
                {
                    depth--;
                    if (depth == 0) { bracketEnd = i; break; }
                }
            }

            if (bracketEnd < 0) return null;

            string arrayContent = json.Substring(bracketStart + 1, bracketEnd - bracketStart - 1);

            // Split by "}{" pattern to find individual keypoint objects
            var keypoints = new List<Keypoint3D>();

            // Find all {...} blocks
            int objStart = arrayContent.IndexOf('{');
            while (objStart >= 0)
            {
                int objEnd = arrayContent.IndexOf('}', objStart);
                if (objEnd < 0) break;

                string objStr = arrayContent.Substring(objStart, objEnd - objStart + 1);

                float x = ExtractFloat(objStr, "x");
                float y = ExtractFloat(objStr, "y");
                float z = ExtractFloat(objStr, "z");
                keypoints.Add(new Keypoint3D(x, y, z));

                objStart = arrayContent.IndexOf('{', objEnd);
            }

            return keypoints.ToArray();
        }

        // -----------------------------------------------------------------------
        // Status management
        // -----------------------------------------------------------------------
        private void SetStatus(ConnectionStatus status, string message)
        {
            Status = status;
            OnConnectionStatusChanged?.Invoke(status, message);
        }

        // -----------------------------------------------------------------------
        // JSON utility wrapper (for JsonUtility compatibility)
        // -----------------------------------------------------------------------
        [Serializable]
        private class SensorMessageWrapper
        {
            public double timestamp;
            public string hall_json;       // JSON-encoded float array
            public string imu_euler_json;
            public string imu_gyro_json;
            public string keypoints_json;  // JSON-encoded Keypoint3D array
            public string gesture;
            public float confidence;
            public string nlp_text;

            public SensorMessage ToSensorMessage()
            {
                return new ManualParseSensorMessage(fullJson);
            }

            private string fullJson;
            public void SetRawJson(string json) { fullJson = json; }
        }
    }

    // =========================================================================
    // Thread-safe queue implementation (no ConcurrentQueue in .NET Standard 2.0)
    // =========================================================================
    public class ThreadSafeQueue<T>
    {
        private readonly Queue<T> _queue = new Queue<T>();
        private readonly object _lock = new object();

        public void Enqueue(T item)
        {
            lock (_lock)
            {
                _queue.Enqueue(item);
            }
        }

        public bool TryDequeue(out T item)
        {
            lock (_lock)
            {
                if (_queue.Count > 0)
                {
                    item = _queue.Dequeue();
                    return true;
                }
                item = default;
                return false;
            }
        }

        public int Count
        {
            get { lock (_lock) { return _queue.Count; } }
        }

        public void Clear()
        {
            lock (_lock)
            {
                _queue.Clear();
            }
        }
    }
}
