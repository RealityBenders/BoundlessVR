using System.Collections.Generic;
using UnityEngine;

public class UIStartPanelSwitch : MonoBehaviour
{
    [Tooltip("Assign each page container (child GameObject) you want to toggle.")]
    public List<GameObject> pages = new List<GameObject>();

    int _current = -1;

    void Awake()
    {
        // Optionally show first page by default
        if (pages.Count > 0) Show(0);
    }

    public void Show(int index)
    {
        if (index < 0 || index >= pages.Count) return;
        _current = index;

        for (int i = 0; i < pages.Count; i++)
            if (pages[i]) pages[i].SetActive(i == index);
    }

    // Optional helper if you prefer names from the Button
    public void ShowByName(string pageName)
    {
        for (int i = 0; i < pages.Count; i++)
            if (pages[i] && pages[i].name == pageName) { Show(i); return; }
    }
}