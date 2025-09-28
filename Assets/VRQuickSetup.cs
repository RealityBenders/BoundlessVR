#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;
using UnityEngine.UI;

public static class VRQuickSetup
{
    [MenuItem("Tools/VR/Build Timer HUD + Fade (Top-Right)")]
    public static void BuildHud()
    {
        // Find main camera
        var cam = Camera.main;
        if (!cam) { EditorUtility.DisplayDialog("VR", "No Main Camera found (tagged MainCamera).", "OK"); return; }

        // --- Canvas under camera (world space HUD) ---
        var canvasGO = new GameObject("HUD_Canvas", typeof(Canvas), typeof(CanvasRenderer));
        Undo.RegisterCreatedObjectUndo(canvasGO, "Create VR HUD");
        canvasGO.transform.SetParent(cam.transform, false);
        var canvas = canvasGO.GetComponent<Canvas>();
        canvas.renderMode = RenderMode.WorldSpace;
        canvas.worldCamera = cam;
        var rt = canvasGO.GetComponent<RectTransform>();
        rt.sizeDelta = new Vector2(400, 400);
        canvasGO.transform.localScale = Vector3.one * 0.001f;
        canvasGO.transform.localPosition = new Vector3(0.35f, 0.25f, 1.2f);
        canvasGO.transform.localRotation = Quaternion.identity;

        // Ring background
        var ringBG = CreateImage("RingBG", canvasGO.transform);
        ringBG.rectTransform.sizeDelta = new Vector2(380, 380);
        ringBG.color = new Color(0,0,0,0.5f);
        ringBG.raycastTarget = false;

        // Ring fill (radial)
        var ringFill = CreateImage("RingFill", canvasGO.transform);
        ringFill.rectTransform.sizeDelta = new Vector2(360, 360);
        ringFill.raycastTarget = false;
        ringFill.type = Image.Type.Filled;
        ringFill.fillMethod = Image.FillMethod.Radial360;
        ringFill.fillClockwise = true;
        ringFill.fillAmount = 1f;
        ringFill.color = Color.white;

        // Built-in round sprite
        var knob = AssetDatabase.GetBuiltinExtraResource<Sprite>("UI/Skin/Knob.psd");
        if (knob) { ringBG.sprite = knob; ringFill.sprite = knob; }

        // Time label (legacy Text to avoid TMP import)
        var textGO = new GameObject("TimeLabel", typeof(RectTransform));
        textGO.transform.SetParent(canvasGO.transform, false);
        var label = textGO.AddComponent<Text>();
        label.alignment = TextAnchor.MiddleCenter;
        label.fontSize = 56;
        label.color = Color.white;
        label.text = "1:00";
        label.raycastTarget = false;
        textGO.GetComponent<RectTransform>().sizeDelta = new Vector2(240, 120);

        // Heartbeat + Breathing audio (2D)
        var hb = CreateAudio("HeartbeatAudio", canvasGO.transform);
        var br = CreateAudio("BreathingAudio", canvasGO.transform);

        // --- Full-screen fade QUAD in front of camera (guaranteed headset coverage) ---
        var quad = GameObject.CreatePrimitive(PrimitiveType.Quad);
        Undo.RegisterCreatedObjectUndo(quad, "Create Fade Quad");
        quad.name = "ScreenFadeQuad";
        quad.transform.SetParent(cam.transform, false);
        quad.transform.localPosition = new Vector3(0, 0, 0.35f);
        quad.transform.localRotation = Quaternion.identity;
        quad.transform.localScale = new Vector3(5f, 5f, 1f);
        Object.DestroyImmediate(quad.GetComponent<Collider>());

        // Unlit black material with alpha 0
        var mat = new Material(Shader.Find("Unlit/Color"));
        mat.color = new Color(0,0,0,0);
        quad.GetComponent<MeshRenderer>().sharedMaterial = mat;

        // Add fade controller
        var fade = quad.AddComponent<CameraFullBlackFade>();

        // Add timer + fx scripts to the canvas and wire them
        var timer = canvasGO.AddComponent<VRCircularCountdown>();
        timer.durationSeconds = 60f;
        timer.autoStart = true;
        timer.ringFill = ringFill;
        timer.timeLabel = label;

        var fx = canvasGO.AddComponent<EndGameFXDirector>();
        fx.countdown = timer;
        fx.screenFade = fade;
        fx.heartbeat = hb;
        fx.breathing = br;

        Selection.activeObject = canvasGO;
        EditorGUIUtility.PingObject(canvasGO);
        EditorUtility.DisplayDialog("VR", "HUD + Fade created and wired.\nPress Play.", "OK");
    }

    static Image CreateImage(string name, Transform parent)
    {
        var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
        go.transform.SetParent(parent, false);
        return go.GetComponent<Image>();
    }

    static AudioSource CreateAudio(string name, Transform parent)
    {
        var go = new GameObject(name, typeof(AudioSource));
        go.transform.SetParent(parent, false);
        var a = go.GetComponent<AudioSource>();
        a.playOnAwake = false; a.loop = true; a.spatialBlend = 0f; a.volume = 0f;
        return a;
    }
}
#endif
