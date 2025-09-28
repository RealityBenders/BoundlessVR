using UnityEngine;

[AddComponentMenu("Fire/Ground Snapper")]
public class GroundSnapper : MonoBehaviour
{
    public LayerMask groundMask;
    public float castHeight = 2f;
    public float maxSnapDistance = 10f;
    public float surfaceOffset = 0.02f;
    public bool alignToSurfaceNormal = false;
    public bool followContinuously = false;
    public float followInterval = 0.2f;

    float _t;

    void Start() => Snap();

    void Update()
    {
        if (!followContinuously) return;
        _t += Time.deltaTime;
        if (_t >= followInterval)
        {
            _t = 0f;
            Snap();
        }
    }

    void Snap()
    {
        Vector3 origin = transform.position + Vector3.up * castHeight;
        if (Physics.Raycast(origin, Vector3.down, out var hit, castHeight + maxSnapDistance, groundMask, QueryTriggerInteraction.Ignore))
        {
            transform.position = hit.point + hit.normal * surfaceOffset;

            if (alignToSurfaceNormal)
            {
                var align = Quaternion.FromToRotation(Vector3.up, hit.normal);
                transform.rotation = align * Quaternion.Euler(0, transform.eulerAngles.y, 0);
            }

            // Parent to moving floor so it rides along
            transform.SetParent(hit.collider.transform, true);
        }
    }
}
