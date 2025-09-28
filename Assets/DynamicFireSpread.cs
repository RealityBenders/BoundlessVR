using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

[AddComponentMenu("Fire/Dynamic Fire Spread")]
public class DynamicFireSpread : MonoBehaviour
{
    [Header("Prefab & Layers")]
    [Tooltip("Assign a *Project* prefab asset (not a scene instance).")]
    public GameObject firePrefab;               // e.g. Mirza's pf_vfx-inf_psys_demo or your 'Fire' prefab asset
    [Tooltip("Select the Ground layer here. Your floor(s) must be on this layer and have colliders.")]
    public LayerMask groundMask;                // set to Ground

    [Header("Area To Populate (bounds)")]
    [Tooltip("Root that encloses the playable building area. We read its child Renderers/Colliders to compute a bounding box.")]
    public Transform areaRoot;                  // e.g., GameWalls (optional if you use existing points)

    [Header("Use Existing Points Instead")]
    [Tooltip("If true and 'pointsRoot' has children, use those positions instead of auto-generating.")]
    public bool useExistingPoints = false;
    [Tooltip("Parent of FirePoint_* empties in the scene.")]
    public Transform pointsRoot;                // FirePointsRoot

    [Header("Point Generation (when not using existing)")]
    [Tooltip("How many fires to spawn in total.")]
    public int maxSpawns = 30;
    [Tooltip("Minimum distance between two fires (meters).")]
    public float minSeparation = 1.5f;
    [Tooltip("Sample attempts per point when searching for a valid floor spot.")]
    public int maxSampleAttemptsPerPoint = 40;
    [Tooltip("Ray start height above area bounds when searching for floor.")]
    public float castUp = 3f;
    [Tooltip("Ray down distance when searching for floor.")]
    public float castDown = 30f;

    [Header("Spread Timing")]
    public bool randomizeOrder = true;
    [Tooltip("Seconds between the first two spawns.")]
    public float initialInterval = 4f;
    [Tooltip("Smallest allowed interval after acceleration.")]
    public float minInterval = 0.5f;
    [Tooltip("Each spawn multiplies the current interval by this (e.g., 0.9 = 10% faster each time).")]
    public float intervalDecay = 0.9f;

    [Header("Events")]
    public UnityEvent onFireComplete;

    // Runtime
    readonly List<Vector3> _spawnPositions = new();
    Bounds _areaBounds;
    bool _hasAreaBounds;

    // Debug gizmos
    [Header("Debug")]
    public bool drawPlannedSpawns = true;
    public float gizmoPointRadius = 0.1f;

    void Start()
    {
        if (!firePrefab)
        {
            Debug.LogError("DynamicFireSpread: Fire Prefab not assigned. Drag a prefab asset into the 'firePrefab' field.", this);
            enabled = false;
            return;
        }

        _spawnPositions.Clear();

        if (useExistingPoints && pointsRoot && pointsRoot.childCount > 0)
        {
            // Use manually placed FirePoint_* positions
            for (int i = 0; i < pointsRoot.childCount; i++)
                _spawnPositions.Add(pointsRoot.GetChild(i).position);
        }
        else
        {
            // Need bounds to generate positions
            _hasAreaBounds = ResolveAreaBounds(out _areaBounds);
            if (!_hasAreaBounds)
            {
                Debug.LogError("DynamicFireSpread: Could not resolve area bounds. Assign 'areaRoot' (e.g., GameWalls) or enable 'useExistingPoints' with a Points Root.", this);
                enabled = false;
                return;
            }

            GeneratePointsProcedurally();
        }

        if (_spawnPositions.Count == 0)
        {
            Debug.LogWarning("DynamicFireSpread: No spawn positions found.", this);
            return;
        }

        if (randomizeOrder)
            Shuffle(_spawnPositions);

        StartCoroutine(SpreadRoutine());
    }

    IEnumerator SpreadRoutine()
    {
        float interval = Mathf.Max(0.01f, initialInterval);

        for (int i = 0; i < _spawnPositions.Count; i++)
        {
            SpawnFire(_spawnPositions[i]);
            yield return new WaitForSeconds(interval);
            interval = Mathf.Max(minInterval, interval * intervalDecay);
        }

        onFireComplete?.Invoke();
    }

    void SpawnFire(Vector3 position)
    {
        var go = Instantiate(firePrefab, position, Quaternion.identity);

        // Ensure each instance hugs the floor (even if prefab is offset)
        var snapper = go.GetComponent<GroundSnapper>();
        if (!snapper) snapper = go.AddComponent<GroundSnapper>();
        snapper.groundMask = groundMask;
        snapper.followContinuously = false; // set true if your floor moves
        snapper.surfaceOffset = 0.02f;
    }

    void GeneratePointsProcedurally()
    {
        var chosen = new List<Vector3>();

        for (int n = 0; n < maxSpawns; n++)
        {
            bool placed = false;

            for (int attempt = 0; attempt < maxSampleAttemptsPerPoint; attempt++)
            {
                // sample random XZ within bounds
                float x = Random.Range(_areaBounds.min.x, _areaBounds.max.x);
                float z = Random.Range(_areaBounds.min.z, _areaBounds.max.z);
                Vector3 castStart = new Vector3(x, _areaBounds.max.y + castUp, z);

                if (Physics.Raycast(castStart, Vector3.down, out RaycastHit hit, castUp + castDown, groundMask, QueryTriggerInteraction.Ignore))
                {
                    Vector3 candidate = hit.point;

                    // separation check
                    bool farEnough = true;
                    for (int j = 0; j < chosen.Count; j++)
                    {
                        if (Vector3.SqrMagnitude(chosen[j] - candidate) < (minSeparation * minSeparation))
                        {
                            farEnough = false; break;
                        }
                    }

                    if (farEnough)
                    {
                        chosen.Add(candidate);
                        placed = true;
                        break;
                    }
                }
            }

            if (!placed)
                continue; // couldn't find a spot for this index
        }

        _spawnPositions.AddRange(chosen);
    }

    bool ResolveAreaBounds(out Bounds b)
    {
        b = new Bounds();
        if (!areaRoot) return false;

        Renderer[] renders = areaRoot.GetComponentsInChildren<Renderer>();
        Collider[] cols    = areaRoot.GetComponentsInChildren<Collider>();

        bool hasAny = false;
        foreach (var r in renders)
        {
            if (!hasAny) { b = r.bounds; hasAny = true; }
            else b.Encapsulate(r.bounds);
        }
        foreach (var c in cols)
        {
            if (!hasAny) { b = c.bounds; hasAny = true; }
            else b.Encapsulate(c.bounds);
        }
        return hasAny;
    }

    void Shuffle(List<Vector3> list)
    {
        for (int i = list.Count - 1; i > 0; i--)
        {
            int k = Random.Range(0, i + 1);
            (list[i], list[k]) = (list[k], list[i]);
        }
    }

    void OnDrawGizmosSelected()
    {
        // Draw area bounds
        if (areaRoot)
        {
            if (Application.isPlaying ? _hasAreaBounds : ResolveAreaBounds(out _areaBounds))
            {
                Gizmos.DrawWireCube(_areaBounds.center, _areaBounds.size);
            }
        }

        // Draw planned spawn points during Play
        if (Application.isPlaying && drawPlannedSpawns)
        {
            for (int i = 0; i < _spawnPositions.Count; i++)
            {
                Gizmos.DrawSphere(_spawnPositions[i], gizmoPointRadius);
            }
        }
    }
}

