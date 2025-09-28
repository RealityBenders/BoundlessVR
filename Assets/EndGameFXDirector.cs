using System.Collections;
using UnityEngine;

public class EndGameFXDirector : MonoBehaviour
{
    [Header("Links")]
    public VRCircularCountdown countdown;   // assign the component on your HUD Canvas
    public CameraFullBlackFade screenFade;  // assign ScreenFadeQuad (with CameraFullBlackFade)
    public AudioSource heartbeat;           // optional, assign
    public AudioSource breathing;           // optional, assign

    [Header("Settings")]
    public float targetPreAlpha = 0.6f;     // dim level during warning
    public float finalFadeDuration = 1.2f;  // seconds to full black
    public bool pauseOnFinish = true;

    void OnEnable()
    {
        if (countdown)
        {
            countdown.onPreEndStart.AddListener(OnPreEndStart);
            countdown.onTimerFinished.AddListener(OnTimeUp);
        }
        else
        {
            Debug.LogWarning("EndGameFXDirector: countdown not assigned.");
        }
    }

    void OnDisable()
    {
        if (countdown)
        {
            countdown.onPreEndStart.RemoveListener(OnPreEndStart);
            countdown.onTimerFinished.RemoveListener(OnTimeUp);
        }
    }

    void OnPreEndStart()
    {
        // Start audio ramp
        if (heartbeat) { heartbeat.volume = 0f; heartbeat.loop = true; if (!heartbeat.isPlaying) heartbeat.Play(); }
        if (breathing) { breathing.volume = 0f; breathing.loop = true; if (!breathing.isPlaying) breathing.Play(); }
        StartCoroutine(AudioRamp(countdown.preEndThresholdSeconds));

        // Partial dim of full headset view
        if (screenFade) screenFade.FadeTo(targetPreAlpha, countdown.preEndThresholdSeconds);
    }

    void OnTimeUp()
    {
        StartCoroutine(FadeToBlackAndPause());
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

    IEnumerator FadeToBlackAndPause()
    {
        if (screenFade) screenFade.FadeTo(1f, finalFadeDuration);
        float t = 0f;
        while (t < finalFadeDuration) { t += Time.unscaledDeltaTime; yield return null; }
        if (pauseOnFinish) Time.timeScale = 0f;
    }
}
