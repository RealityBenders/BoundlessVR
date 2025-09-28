using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
#if TMP_PRESENT || UNITY_TEXTMESHPRO
using TMPro;
#endif

public class VRCircularCountdown : MonoBehaviour
{
    [Header("Timer")]
    public float durationSeconds = 60f;
    public bool autoStart = true;

    [Header("Pre-End Warning")]
    public float preEndThresholdSeconds = 4f;   // when the 'panic' starts
    public UnityEvent onPreEndStart;            // fired once when remaining <= threshold
    public UnityEvent onTimerFinished;          // fired when remaining == 0

    [Header("UI References")]
    public Image ringFill; // Image set to Filled → Radial 360
#if TMP_PRESENT || UNITY_TEXTMESHPRO
    public TextMeshProUGUI timeLabelTMP;
#else
    public Text timeLabel;
#endif

    [Header("Colors")]
    public Color normalColor = Color.white;
    public Color warningColor = new Color(1f, 0.4f, 0.2f);
    public float warningThreshold = 10f; // seconds

    public float RemainingSeconds => _remaining;

    float _remaining;
    bool _running;
    bool _preEndFired;

    void Awake()
    {
        // Auto-find if user forgot to assign
        if (!ringFill) ringFill = GetComponentInChildren<Image>(true);
#if TMP_PRESENT || UNITY_TEXTMESHPRO
        if (!timeLabelTMP) timeLabelTMP = GetComponentInChildren<TextMeshProUGUI>(true);
#else
        if (!timeLabel)    timeLabel    = GetComponentInChildren<Text>(true);
#endif
    }

    void Start()
    {
        ResetTimer();
        if (autoStart) StartTimer();
    }

    public void ResetTimer()
    {
        _remaining = Mathf.Max(0f, durationSeconds);
        _running = false;
        _preEndFired = false;
        UpdateUI();
    }

    public void StartTimer() => _running = true;
    public void StopTimer()  => _running = false;

    void Update()
    {
        if (!_running) return;

        _remaining -= Time.deltaTime;

        if (!_preEndFired && _remaining > 0f && _remaining <= preEndThresholdSeconds)
        {
            _preEndFired = true;
            onPreEndStart?.Invoke();
        }

        if (_remaining <= 0f)
        {
            _remaining = 0f;
            _running = false;
            UpdateUI();
            onTimerFinished?.Invoke();
            return;
        }

        UpdateUI();
    }

    void UpdateUI()
    {
        float t = durationSeconds <= 0.001f ? 0f : _remaining / durationSeconds;

        if (ringFill)
        {
            ringFill.fillAmount = t;
            ringFill.color = (_remaining <= warningThreshold) ? warningColor : normalColor;
        }

#if TMP_PRESENT || UNITY_TEXTMESHPRO
        if (timeLabelTMP) timeLabelTMP.text = FormatTime(_remaining);
#else
        if (timeLabel)    timeLabel.text    = FormatTime(_remaining);
#endif
    }

    string FormatTime(float s)
    {
        int total = Mathf.CeilToInt(s);
        int min = total / 60;
        int sec = total % 60;
        return $"{min:0}:{sec:00}";
    }
}
