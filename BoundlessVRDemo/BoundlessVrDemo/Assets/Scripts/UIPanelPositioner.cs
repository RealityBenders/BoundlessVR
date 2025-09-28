using UnityEngine;

public class UIPanelPositioner : MonoBehaviour
{
    [Header("Placement Settings")]
    [Tooltip("Distance in meters in front of the player’s camera.")]
    public float distance = 2.0f;

    [Tooltip("Vertical offset in meters above/below eye level.")]
    public float heightOffset = 0.0f;

    [Tooltip("Rotate the panel so it faces the player.")]
    public bool faceCamera = true;

    void OnEnable()
    {
        Camera cam = Camera.main; // XR Main Camera should have the "MainCamera" tag
        if (cam == null)
        {
            Debug.LogWarning("No Main Camera found! Tag your XR camera as 'MainCamera'.");
            return;
        }

        // Forward vector projected on the ground plane (so UI stays level)
        Vector3 forward = cam.transform.forward;
        forward.y = 0;
        forward.Normalize();

        // Position panel in front of camera with offset
        transform.position = cam.transform.position + forward * distance + Vector3.up * heightOffset;

        if (faceCamera)
        {
            // Make panel face the camera (upright, no tilting backwards/sideways)
            transform.rotation = Quaternion.LookRotation(forward, Vector3.up);
        }
    }
}
