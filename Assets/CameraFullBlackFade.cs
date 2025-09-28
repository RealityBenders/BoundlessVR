using System.Collections;
using UnityEngine;

[RequireComponent(typeof(Renderer))]
public class CameraFullBlackFade : MonoBehaviour
{
    public float alpha { get; private set; } = 0f; // 0..1
    Material _mat;

    void Awake()
    {
        var r = GetComponent<Renderer>();
        _mat = r.material; // instance
        SetAlpha(0f);
    }

    public void SetAlpha(float a)
    {
        alpha = Mathf.Clamp01(a);
        var c = _mat.color;
        c.a = alpha;
        _mat.color = c;
    }

    public void FadeTo(float target, float duration)
    {
        StopAllCoroutines();
        StartCoroutine(FadeRoutine(target, duration));
    }

    IEnumerator FadeRoutine(float target, float duration)
    {
        float start = alpha;
        float t = 0f;
        while (t < duration)
        {
            t += Time.unscaledDeltaTime;
            SetAlpha(Mathf.Lerp(start, target, t / duration));
            yield return null;
        }
        SetAlpha(target);
    }
}
