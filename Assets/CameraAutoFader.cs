using System.Collections;
using UnityEngine;
using UnityEngine.UI;

[DisallowMultipleComponent]
public class CameraAutoFader : MonoBehaviour
{
    [Header("Links")]
    public VRCircularCountdown countdown;   // drag your VRCircularCountdown (on HUD_Canvas)
    public AudioSource heartbeat;           // optional
    public AudioSource breathing;           // optional

    [Header("Fade Settings")]
    public float warnStartAlpha = 0.6f;     // how dark during the warning window
    public float finalFadeDuration = 1.2f;  // seconds from warn to full black
    public bool pauseOnFinish = true;

    Canvas _canvas;
    Image  _black;
    CanvasGroup _cg;

    void Awake()
    {
        // Create a Screen Space - Camera canvas attached to THIS camera
        _canvas = new GameObject("AutoFadeCanvas", typeof(Canvas), typeof(CanvasScaler)).GetComponent<Canvas>();
        _canvas.transform.SetParent(transform, false);
        _canvas.renderMode   = RenderMode.ScreenSpaceCamera;
        _canvas.worldCamera  = GetComponent<Camera>();
        _canvas.sortingOrder = 9999; // always on top

        // Fullscreen black Image + CanvasGroup
        _black = new GameObject("Black", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image)).GetComponent<Image>();
        _black.transform.SetParent(_canvas.transform, false);
        var rt = _black.rectTransform;
        rt.anchorMin = Vector2.zero; rt.anchorMax = Vector2.one;
        rt.offsetMin = Vector2.zero; rt.offsetMax = Vector2.zero;

        _black.color = Color.black;
        _black.raycastTarget = false;

        _cg = _black.gameObject.AddComponent<CanvasGroup>();
        _cg.alpha = 0f; // start transparent
    }

    void OnEnable()
    {
        if (!countdown)
        {
            Debug.LogWarning("CameraAutoFader: countdown not assigned (drag your VRCircularCountdown).");
            return;
        }

        countdown.onPreEndStart.AddListener(OnPreEndStart);
        countdown.onTimerFinished.AddListener(OnTimeUp);
    }

    void OnDisable()
    {
        if (!countdown) return;
        countdown.onPreEndStart.RemoveListener(OnPreEndStart);
        countdown.onTimerFinished.RemoveListener(OnTimeUp);
    }

    void OnPreEndStart()
    {
        // Start audio ramp
        if (heartbeat) { heartbeat.volume = 0f; heartbeat.loop = true; if (!heartbeat.isPlaying) heartbeat.Play(); }
        if (breathing) { breathing.volume = 0f; breathing.loop = true; if (!breathing.isPlaying) breathing.Play(); }
        StartCoroutine(AudioRamp(countdown.preEndThresholdSeconds));

        // Dim to warnStartAlpha over the remaining seconds (e.g., last 4s)
        StartCoroutine(FadeTo(warnStartAlpha, countdown.preEndThresholdSeconds));
    }

    void OnTimeUp()
    {
        // Finish fade to full black, then optionally pause
        StartCoroutine(FadeTo(1f, finalFadeDuration, () =>
        {
            if (pauseOnFinish) Time.timeScale = 0f;
        }));
    }

    IEnumerator FadeTo(float target, float duration, System.Action onDone = null)
    {
        float start = _cg.alpha;
        float t = 0f;
        while (t < duration)
        {
            t += Time.unscaledDeltaTime;
            _cg.alpha = Mathf.Lerp(start, target, t / duration);
            yield return null;
        }
        _cg.alpha = target;
        onDone?.Invoke();
    }

    IEnumerator AudioRamp(float duration)
    {
        float t = 0f;
        while (t < duration)
        {
            t += Time.deltaTime;
            float v = Mathf.Clamp01(t / duration);
            if (heartbeat) heartbeat.volume = v;
            if (breathing) breathing.volume = v;
            yield return null;
        }
    }
}

