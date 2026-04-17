using UnityEngine;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Google.Protobuf; // Requires Protobuf DLL in Unity

public class UDPReceiver : MonoBehaviour
{
    public int port = 8888;
    private UdpClient client;
    private Thread receiveThread;
    
    public GloveData latestData; // Generated from glove_data.proto
    private bool dataReceived = false;

    void Start()
    {
        client = new UdpClient(port);
        receiveThread = new Thread(new ThreadStart(ReceiveData));
        receiveThread.IsBackground = true;
        receiveThread.Start();
    }

    private void ReceiveData()
    {
        while (true)
        {
            try
            {
                IPEndPoint anyIP = new IPEndPoint(IPAddress.Any, 0);
                byte[] data = client.Receive(ref anyIP);
                
                latestData = GloveData.Parser.ParseFrom(data);
                dataReceived = true;
            }
            catch (System.Exception e)
            {
                Debug.LogError(e.ToString());
            }
        }
    }

    public bool HasNewData()
    {
        if (dataReceived)
        {
            dataReceived = false;
            return true;
        }
        return false;
    }

    void OnApplicationQuit()
    {
        if (receiveThread != null) receiveThread.Abort();
        client.Close();
    }
}
