// =============================================================================
// UDPReceiver.cs
// EdgeAI DataGlove V3 — Unity L3 Skeleton
// Alternative receiver that listens on a UDP port for Protobuf-serialized
// GloveData messages from the Python inference pipeline.
//
// Protocol:
//   - Port: 9999 (configurable)
//   - Payload: Protobuf-encoded GloveData message
//   - Expected message size: ~200–400 bytes
//
// Protobuf schema (glove_data.proto):
//   message GloveData {
//       double timestamp   = 1;
//       repeated float hall = 2;        // 15 flex sensor values
//       repeated float imu_euler = 3;   // roll, pitch, yaw
//       repeated float imu_gyro  = 4;   // angular velocity
//       repeated Keypoint keypoints = 5;
//       string gesture     = 6;
//       float  confidence  = 7;
//       string nlp_text    = 8;
//   }
//   message Keypoint {
//       float x = 1;
//       float y = 2;
//       float z = 3;
//   }
//
// Note: This file uses manual binary parsing for the protobuf wire format
// to avoid requiring a .proto compilation step in the Unity project.
// For production use, consider using Google.Protobuf Unity package.
// =============================================================================

using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;

namespace EdgeAI.DataGlove.Networking
{
    /// <summary>
    /// UDP receiver that listens for Protobuf-encoded glove sensor data
    /// and dispatches parsed SensorMessage objects on the main Unity thread.
    /// </summary>
    public class UDPReceiver : MonoBehaviour
    {
        // -----------------------------------------------------------------------
        // Inspector fields
        // -----------------------------------------------------------------------
        [Header("UDP Settings")]
        [Tooltip("Local UDP port to listen on")]
        [SerializeField] private int listenPort = 9999;

        [Tooltip("Maximum UDP payload size in bytes")]
        [SerializeField] private int maxPacketSize = 2048;

        [Tooltip("Allowed remote endpoint IP (empty = accept from any)")]
        [SerializeField] private string allowedRemoteIP = "";

        [Header("Performance")]
        [Tooltip("Maximum messages to process per frame")]
        [SerializeField] private int maxMessagesPerFrame = 5;

        [Header("Debug")]
        [Tooltip("Log all received packets to console")]
        [SerializeField] private bool verboseLogging = false;

        // -----------------------------------------------------------------------
        // Public events
        // -----------------------------------------------------------------------
        /// <summary>Fired when a complete SensorMessage is received and parsed.</summary>
        public event Action<SensorMessage> OnMessageReceived;

        /// <summary>Fired when connection state changes.</summary>
        public event Action<ConnectionStatus, string> OnConnectionStatusChanged;

        /// <summary>Fired on any parsing or network error.</summary>
        public event Action<string> OnError;

        // -----------------------------------------------------------------------
        // Public properties
        // -----------------------------------------------------------------------
        public ConnectionStatus Status { get; private set; } = ConnectionStatus.Disconnected;
        public bool IsListening => Status == ConnectionStatus.Connected;
        public int PacketsPerSecond { get; private set; }
        public int TotalPacketsReceived { get; private set; }
        public int ParseErrors { get; private set; }
        public float CurrentLatencyMs { get; private set; }

        // -----------------------------------------------------------------------
        // Private state
        // -----------------------------------------------------------------------
        private ThreadSafeQueue<byte[]> _packetQueue = new ThreadSafeQueue<byte[]>();
        private UdpClient _udpClient;
        private Thread _receiveThread;
        private volatile bool _isRunning;

        private int _packetCount;
        private float _ppsAccumulator;
        private float _ppsInterval = 1f;
        private IPEndPoint _remoteEndpoint;

        // -----------------------------------------------------------------------
        // Unity lifecycle
        // -----------------------------------------------------------------------
        private void Awake()
        {
            DontDestroyOnLoad(this.gameObject);
        }

        private void Update()
        {
            ProcessPacketQueue();

            // Track packets per second
            _ppsAccumulator += Time.deltaTime;
            if (_ppsAccumulator >= _ppsInterval)
            {
                PacketsPerSecond = _packetCount;
                _packetCount = 0;
                _ppsAccumulator -= _ppsInterval;
            }
        }

        private void OnDestroy()
        {
            StopListening();
        }

        private void OnApplicationQuit()
        {
            StopListening();
        }

        // -----------------------------------------------------------------------
        // Public methods
        // -----------------------------------------------------------------------

        /// <summary>Start listening on the configured UDP port.</summary>
        public void StartListening()
        {
            if (_isRunning)
            {
                Debug.LogWarning("[UDPReceiver] Already listening.");
                return;
            }

            try
            {
                _udpClient = new UdpClient(listenPort);
                _udpClient.EnableBroadcast = true;
                _remoteEndpoint = new IPEndPoint(IPAddress.Any, 0);

                _isRunning = true;
                _receiveThread = new Thread(ReceiveLoop)
                {
                    Name = "UDPReceiver",
                    IsBackground = true
                };
                _receiveThread.Start();

                SetStatus(ConnectionStatus.Connected, $"Listening on UDP port {listenPort}");
                Debug.Log($"[UDPReceiver] Started listening on UDP port {listenPort}");
            }
            catch (SocketException ex)
            {
                SetStatus(ConnectionStatus.Error, $"Socket error: {ex.Message}");
                Debug.LogError($"[UDPReceiver] Failed to start: {ex.Message}");
            }
        }

        /// <summary>Stop listening and release resources.</summary>
        public void StopListening()
        {
            _isRunning = false;
            SetStatus(ConnectionStatus.Disconnected, "Stopped listening");

            try
            {
                _udpClient?.Close();
            }
            catch { /* swallow */ }
            _udpClient = null;

            if (_receiveThread != null && _receiveThread.IsAlive)
            {
                if (!_receiveThread.Join(1000))
                {
                    Debug.LogWarning("[UDPReceiver] Receive thread did not exit gracefully.");
                }
                _receiveThread = null;
            }
        }

        /// <summary>Change the listen port at runtime (requires restart).</summary>
        public void SetPort(int port)
        {
            if (IsListening)
            {
                Debug.LogWarning("[UDPReceiver] Stop listening before changing port.");
                return;
            }
            listenPort = port;
        }

        // -----------------------------------------------------------------------
        // Receive loop (background thread)
        // -----------------------------------------------------------------------
        private void ReceiveLoop()
        {
            while (_isRunning)
            {
                try
                {
                    byte[] data = _udpClient.Receive(ref _remoteEndpoint);
                    TotalPacketsReceived++;

                    if (verboseLogging)
                    {
                        Debug.Log($"[UDPReceiver] Received {data.Length} bytes from {_remoteEndpoint}");
                    }

                    // Enqueue for main-thread processing
                    _packetQueue.Enqueue(data);
                    _packetCount++;
                }
                catch (SocketException)
                {
                    // Expected when _udpClient.Close() is called
                    if (_isRunning)
                    {
                        Debug.LogWarning("[UDPReceiver] Socket closed unexpectedly.");
                    }
                    break;
                }
                catch (ObjectDisposedException)
                {
                    // Expected during shutdown
                    break;
                }
                catch (Exception ex)
                {
                    Debug.LogError($"[UDPReceiver] Receive error: {ex.Message}");
                }
            }
        }

        // -----------------------------------------------------------------------
        // Main-thread packet processing
        // -----------------------------------------------------------------------
        private void ProcessPacketQueue()
        {
            int processed = 0;
            while (processed < maxMessagesPerFrame && _packetQueue.TryDequeue(out byte[] data))
            {
                try
                {
                    SensorMessage msg = ParseProtobuf(data);
                    if (msg != null && msg.IsValid())
                    {
                        // Estimate latency
                        if (msg.timestamp > 0)
                        {
                            double now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                            CurrentLatencyMs = Mathf.Abs((float)((now - msg.timestamp) * 1000.0));
                        }

                        OnMessageReceived?.Invoke(msg);
                    }
                    else
                    {
                        ParseErrors++;
                        if (verboseLogging)
                        {
                            Debug.LogWarning("[UDPReceiver] Invalid or empty message received.");
                        }
                    }
                }
                catch (Exception ex)
                {
                    ParseErrors++;
                    Debug.LogError($"[UDPReceiver] Parse error: {ex.Message}");
                    OnError?.Invoke($"Parse error: {ex.Message}");
                }

                processed++;
            }
        }

        // -----------------------------------------------------------------------
        // Protobuf wire-format parser
        //
        // Protobuf wire types:
        //   0 = Varint
        //   1 = 64-bit (fixed64, sfixed64, double)
        //   2 = Length-delimited (string, bytes, embedded messages, packed repeated fields)
        //   5 = 32-bit (fixed32, sfixed32, float)
        //
        // Field number = field_tag >> 3
        // Wire type    = field_tag & 0x07
        // -----------------------------------------------------------------------
        private SensorMessage ParseProtobuf(byte[] data)
        {
            var msg = new SensorMessage();
            int offset = 0;

            while (offset < data.Length)
            {
                // Read field tag (varint)
                uint fieldTag;
                int bytesRead;
                if (!TryReadVarint(data, offset, out fieldTag, out bytesRead))
                    break;
                offset += bytesRead;

                uint fieldNumber = fieldTag >> 3;
                uint wireType = fieldTag & 0x07;

                switch (fieldNumber)
                {
                    case 1: // timestamp (double, wire type 1)
                        if (wireType == 1 && offset + 8 <= data.Length)
                        {
                            msg.timestamp = BitConverter.ToDouble(data, offset);
                            offset += 8;
                        }
                        else
                        {
                            SkipWireType(data, ref offset, wireType);
                        }
                        break;

                    case 2: // hall (repeated float, packed wire type 2)
                        msg.hall = ReadPackedFloats(data, ref offset, wireType);
                        break;

                    case 3: // imu_euler (repeated float, packed wire type 2)
                        msg.imu_euler = ReadPackedFloats(data, ref offset, wireType);
                        break;

                    case 4: // imu_gyro (repeated float, packed wire type 2)
                        msg.imu_gyro = ReadPackedFloats(data, ref offset, wireType);
                        break;

                    case 5: // keypoints (repeated Keypoint message, wire type 2)
                        if (wireType == 2)
                        {
                            // Read message length
                            uint msgLen;
                            if (TryReadVarint(data, offset, out msgLen, out bytesRead))
                            {
                                offset += bytesRead;
                                // For repeated embedded messages, we accumulate
                                // Each Keypoint is a separate field=5 entry
                                var kp = ParseKeypointMessage(data, offset, (int)msgLen);
                                if (kp != null)
                                {
                                    if (msg.keypoints == null)
                                        msg.keypoints = new Keypoint3D[] { kp };
                                    else
                                    {
                                        var list = new List<Keypoint3D>(msg.keypoints) { kp };
                                        msg.keypoints = list.ToArray();
                                    }
                                }
                                offset += (int)msgLen;
                            }
                        }
                        else
                        {
                            SkipWireType(data, ref offset, wireType);
                        }
                        break;

                    case 6: // gesture (string, wire type 2)
                        msg.gesture = ReadString(data, ref offset, wireType);
                        break;

                    case 7: // confidence (float, wire type 5)
                        if (wireType == 5 && offset + 4 <= data.Length)
                        {
                            msg.confidence = BitConverter.ToSingle(data, offset);
                            offset += 4;
                        }
                        else
                        {
                            SkipWireType(data, ref offset, wireType);
                        }
                        break;

                    case 8: // nlp_text (string, wire type 2)
                        msg.nlp_text = ReadString(data, ref offset, wireType);
                        break;

                    default:
                        // Unknown field — skip
                        SkipWireType(data, ref offset, wireType);
                        break;
                }
            }

            return msg;
        }

        // -----------------------------------------------------------------------
        // Protobuf primitive readers
        // -----------------------------------------------------------------------

        private static Keypoint3D ParseKeypointMessage(byte[] data, int start, int length)
        {
            var kp = new Keypoint3D();
            int offset = start;
            int end = start + length;

            while (offset < end)
            {
                uint fieldTag;
                int bytesRead;
                if (!TryReadVarint(data, offset, out fieldTag, out bytesRead))
                    break;
                offset += bytesRead;

                uint fieldNumber = fieldTag >> 3;
                uint wireType = fieldTag & 0x07;

                if (wireType == 5 && offset + 4 <= end)
                {
                    float val = BitConverter.ToSingle(data, offset);
                    offset += 4;

                    switch (fieldNumber)
                    {
                        case 1: kp.x = val; break;
                        case 2: kp.y = val; break;
                        case 3: kp.z = val; break;
                    }
                }
                else
                {
                    SkipWireType(data, ref offset, wireType);
                }
            }

            return kp;
        }

        private static bool TryReadVarint(byte[] data, int offset, out uint value, out int bytesRead)
        {
            value = 0;
            bytesRead = 0;
            int shift = 0;

            while (offset + bytesRead < data.Length)
            {
                byte b = data[offset + bytesRead];
                value |= (uint)(b & 0x7F) << shift;
                bytesRead++;
                shift += 7;

                if ((b & 0x80) == 0)
                    return true;

                if (shift >= 35)
                    return false; // varint too long
            }

            return false;
        }

        private static float[] ReadPackedFloats(byte[] data, ref int offset, uint wireType)
        {
            if (wireType != 2)
            {
                SkipWireType(data, ref offset, wireType);
                return new float[0];
            }

            uint length;
            int bytesRead;
            if (!TryReadVarint(data, offset, out length, out bytesRead))
            {
                SkipWireType(data, ref offset, wireType);
                return new float[0];
            }
            offset += bytesRead;

            if (length == 0 || offset + length > data.Length)
            {
                return new float[0];
            }

            int count = (int)length / 4; // Each float = 4 bytes
            var result = new float[count];

            for (int i = 0; i < count; i++)
            {
                result[i] = BitConverter.ToSingle(data, offset + i * 4);
            }

            offset += (int)length;
            return result;
        }

        private static string ReadString(byte[] data, ref int offset, uint wireType)
        {
            if (wireType != 2)
            {
                SkipWireType(data, ref offset, wireType);
                return "";
            }

            uint length;
            int bytesRead;
            if (!TryReadVarint(data, offset, out length, out bytesRead))
            {
                SkipWireType(data, ref offset, wireType);
                return "";
            }
            offset += bytesRead;

            if (length == 0 || offset + length > data.Length)
            {
                return "";
            }

            string str = System.Text.Encoding.UTF8.GetString(data, offset, (int)length);
            offset += (int)length;
            return str;
        }

        private static void SkipWireType(byte[] data, ref int offset, uint wireType)
        {
            switch (wireType)
            {
                case 0: // Varint
                    while (offset < data.Length)
                    {
                        if ((data[offset] & 0x80) == 0)
                        {
                            offset++;
                            break;
                        }
                        offset++;
                    }
                    break;

                case 1: // 64-bit
                    offset += 8;
                    break;

                case 2: // Length-delimited
                    uint length;
                    int bytesRead;
                    if (TryReadVarint(data, offset, out length, out bytesRead))
                    {
                        offset += bytesRead + (int)length;
                    }
                    else
                    {
                        offset = data.Length; // Fallback: skip to end
                    }
                    break;

                case 5: // 32-bit
                    offset += 4;
                    break;

                default:
                    // Unknown wire type — advance to end
                    offset = data.Length;
                    break;
            }
        }

        // -----------------------------------------------------------------------
        // Status management
        // -----------------------------------------------------------------------
        private void SetStatus(ConnectionStatus status, string message)
        {
            Status = status;
            OnConnectionStatusChanged?.Invoke(status, message);
        }
    }
}
