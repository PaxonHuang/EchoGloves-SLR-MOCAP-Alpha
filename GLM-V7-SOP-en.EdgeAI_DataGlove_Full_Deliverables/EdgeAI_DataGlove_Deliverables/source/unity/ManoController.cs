using UnityEngine;
using System.Collections.Generic;

public class ManoController : MonoBehaviour
{
    public UDPReceiver udpReceiver;
    public Transform wristJoint;
    public List<Transform> fingerJoints; // 15 joints (3 per finger)

    // Eq 17, 18 Mapping Parameters
    private float[] theta_mano = new float[48];

    void Update()
    {
        if (udpReceiver != null && udpReceiver.latestData != null)
        {
            UpdateHandPose(udpReceiver.latestData);
        }
    }

    void UpdateHandPose(GloveData data)
    {
        // 1. Update Wrist Orientation (IMU Euler/Quat)
        // Eq 18: Apply BNO085 orientation
        float roll = data.ImuFeatures[0];
        float pitch = data.ImuFeatures[1];
        float yaw = data.ImuFeatures[2];
        wristJoint.localRotation = Quaternion.Euler(pitch, yaw, roll);

        // 2. Update Finger Flexion (Hall Sensors)
        // Eq 17: Map 15 Hall features to MANO joints
        for (int i = 0; i < 5; i++)
        {
            float hall_val = data.HallFeatures[i * 3 + 2]; // Use Z-axis for flexion
            
            // Simple linear mapping for demo (MLP would be better)
            float angle = hall_val * 90.0f; 
            
            // Apply to MCP, PIP, DIP joints
            fingerJoints[i * 3].localRotation = Quaternion.Euler(angle, 0, 0);
            fingerJoints[i * 3 + 1].localRotation = Quaternion.Euler(angle * 0.7f, 0, 0);
            fingerJoints[i * 3 + 2].localRotation = Quaternion.Euler(angle * 0.5f, 0, 0);
        }

        // 3. Smooth movements
        // wristJoint.rotation = Quaternion.Slerp(wristJoint.rotation, targetRot, Time.deltaTime * 10f);
    }
}
