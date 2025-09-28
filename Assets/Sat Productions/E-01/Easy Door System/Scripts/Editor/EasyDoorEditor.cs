namespace EasyDoorSystem
{
    using UnityEditor;
    using UnityEngine;

    [CustomEditor(typeof(EasyDoor))]
    public class EasyDoorEditor : Editor
    {
        private EasyDoor door;
        private SerializedObject serializedDoor;
        private Texture2D logo;

        private bool showSettings = true;
        private bool showTransforms = true;
        private bool showAudio = true;
        private bool showEvents = true;

        private void OnEnable()
        {
            door = (EasyDoor)target;
            if (door == null) return;

            serializedDoor = new SerializedObject(door);
            logo = Resources.Load<Texture2D>("easy-door-system-icon");
        }

        public override void OnInspectorGUI()
        {
            if (door == null || serializedDoor == null)
            {
                EditorGUILayout.HelpBox("EasyDoor reference missing!", MessageType.Error);
                return;
            }

            serializedDoor.Update();

            DrawLogo();
            DrawFooterButtons();
            DrawPlayerSetting();
            DrawMainSettings();
            DrawTransformControls();
            DrawUtilities();
            EditorGUILayout.Space(20);
            DrawAudioSettings();
            DrawEventSettings();

            serializedDoor.ApplyModifiedProperties();
        }

        private void DrawLogo()
        {
            if (!logo) return;

            GUILayout.Space(10);
            Rect rect = GUILayoutUtility.GetRect(300, 60);
            GUI.DrawTexture(rect, logo, ScaleMode.ScaleToFit);
            GUILayout.Space(10);
        }

        private void DrawPlayerSetting()
        {
            EditorGUILayout.BeginVertical("HelpBox");
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("Player"), new GUIContent("Player Object"));
            EditorGUILayout.EndVertical();
            EditorGUILayout.Space();
        }


        private void DrawMainSettings()
        {
            showSettings = EditorGUILayout.Foldout(showSettings, EditorGUIUtility.IconContent("Settings"), true);
            if (!showSettings) return;

            EditorGUILayout.BeginVertical("HelpBox");
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("movementType"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("movementSpeed"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("rotationSpeed"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("autoCloseDelay"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("automaticPlayerDetection"));

            if (door.automaticPlayerDetection)
            {
                EditorGUILayout.PropertyField(serializedDoor.FindProperty("detectionRange"));
                EditorGUILayout.HelpBox("Ensure player has 'Player' tag and has a collider", MessageType.Info);
            }

            EditorGUILayout.EndVertical();
            EditorGUILayout.Space();
        }

        private void DrawTransformControls()
        {
            showTransforms = EditorGUILayout.Foldout(showTransforms, EditorGUIUtility.IconContent("d_Transform Icon"), true);
            if (!showTransforms) return;

            EditorGUILayout.BeginVertical("HelpBox");

            // 🔥 Live Status Box
            GUIStyle stateStyle = new GUIStyle(EditorStyles.helpBox);
            stateStyle.normal.textColor = door.IsOpen ? Color.green : Color.red;
            EditorGUILayout.LabelField($"Current State: {(door.IsOpen ? "Open" : "Closed")}", stateStyle);

            EditorGUILayout.LabelField($"Position: {door.transform.localPosition}");
            EditorGUILayout.LabelField($"Rotation: {door.transform.localEulerAngles}");

            GUILayout.Space(10);

            // 🔹 **Larger & Clearer Save Buttons**
            EditorGUILayout.BeginHorizontal();
            GUI.backgroundColor = new Color(0.2f, 0.8f, 0.2f); // Bright Green for Open State
            if (GUILayout.Button(new GUIContent(" Save Open State", EditorGUIUtility.IconContent("d_SaveAs").image), GUILayout.Height(40)))
            {
                door.SaveCurrentState(true);
                SaveCurrentTransformStates(true);
            }
            GUI.backgroundColor = new Color(0.8f, 0.2f, 0.2f); // Bright Red for Closed State
            if (GUILayout.Button(new GUIContent(" Save Closed State", EditorGUIUtility.IconContent("d_SaveAs").image), GUILayout.Height(40)))
            {
                door.SaveCurrentState(false);
                SaveCurrentTransformStates(false);
            }
            GUI.backgroundColor = Color.white; // Reset Color
            EditorGUILayout.EndHorizontal();

            GUI.backgroundColor = Color.white;

            EditorGUILayout.EndVertical();
            EditorGUILayout.Space();
        }

        private void SaveCurrentTransformStates(bool isOpen)
        {
            if (serializedDoor == null) return;

            SerializedProperty posProp = serializedDoor.FindProperty(isOpen ? "openedPosition" : "closedPosition");
            SerializedProperty rotProp = serializedDoor.FindProperty(isOpen ? "openedRotation" : "closedRotation");

            if (posProp != null) posProp.vector3Value = door.transform.localPosition;
            if (rotProp != null) rotProp.vector3Value = door.transform.localEulerAngles;
        }

        private void DrawAudioSettings()
        {
            showAudio = EditorGUILayout.Foldout(showAudio, EditorGUIUtility.IconContent("AudioSource Icon"), true);
            if (!showAudio) return;

            EditorGUILayout.BeginVertical("HelpBox");
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("doorOpenSound"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("doorCloseSound"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("audioVolume"));
            EditorGUILayout.EndVertical();
            EditorGUILayout.Space();
        }

        private void DrawEventSettings()
        {
            showEvents = EditorGUILayout.Foldout(showEvents, EditorGUIUtility.IconContent("EventSystem Icon"), true);
            if (!showEvents) return;

            EditorGUILayout.BeginVertical("HelpBox");
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("OnDoorOpening"));
            EditorGUILayout.PropertyField(serializedDoor.FindProperty("OnDoorClosed"));
            EditorGUILayout.EndVertical();
            EditorGUILayout.Space();
        }

        private void DrawUtilities()
        {
            EditorGUILayout.LabelField("🛠 Utilities", EditorStyles.boldLabel);

            EditorGUILayout.BeginHorizontal();
            GUI.backgroundColor = Color.green;
            if (GUILayout.Button(EditorGUIUtility.IconContent("d_PlayButton On"), GUILayout.Height(30)))
                door.OpenDoor();

            GUI.backgroundColor = Color.red;
            if (GUILayout.Button(EditorGUIUtility.IconContent("d_PreMatQuad"), GUILayout.Height(30)))
                door.CloseDoor();

            GUI.backgroundColor = Color.white;
            EditorGUILayout.EndHorizontal();
        }

        private void DrawFooterButtons()
        {
            GUILayout.Space(10);
            EditorGUILayout.BeginHorizontal();

            DrawImageButton("unity-icon", "https://prf.hn/l/eOBvBBY/");
            DrawImageButton("discord-icon", "https://discord.gg/EGkgqZdN8f");
            DrawImageButton("sat-icon", "https://satproductions.com/");
            DrawImageButton("youtube-icon", "https://www.youtube.com/@satproductionsofficial");

            EditorGUILayout.EndHorizontal();
            GUILayout.Space(10);
        }

        private void DrawImageButton(string iconName, string url)
        {
            Texture2D icon = Resources.Load<Texture2D>(iconName);
            if (icon != null)
            {
                if (GUILayout.Button(icon, GUILayout.Width(32), GUILayout.Height(32)))
                {
                    Application.OpenURL(url);
                }
            }
            else
            {
                if (GUILayout.Button(iconName, GUILayout.Width(100), GUILayout.Height(32)))
                {
                    Application.OpenURL(url);
                }
            }
        }
    }
}
