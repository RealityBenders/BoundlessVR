using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

public class VRStartMenuGate : MonoBehaviour
{
    [Header("Menu UI")]
    [SerializeField] private GameObject startMenuCanvas;      // your Canvas root (active at launch)

    [Header("UI Pointer (allowed during menu)")]
    [SerializeField] private XRRayInteractor uiRay;           // one hand ray used only for UI

    [Header("Disable these during menu")]
    [SerializeField] private List<Behaviour> locomotionProviders; // ContinuousMove/Turn, TeleportationProvider, LocomotionSystem
    [SerializeField] private List<XRBaseInteractor> gameplayInteractors; // other rays, direct hands, teleport rays

    [Header("Raycast masks")]
    [SerializeField] private LayerMask gameplayRaycastMask = ~0; // what your rays hit in gameplay
    [SerializeField] private LayerMask uiOnlyRaycastMask = 0;    // set to Nothing to block physics during menu

    // storage
    private LayerMask _savedUiRayMask;
    private bool _started;

    void Start()
    {
        // show menu
        if (startMenuCanvas) startMenuCanvas.SetActive(true);

        // block gameplay interactor use
        foreach (var inter in gameplayInteractors) if (inter) inter.enabled = false;

        // disable locomotion
        foreach (var b in locomotionProviders) if (b) b.enabled = false;

        // make the UI ray only touch UI (not 3D)
        if (uiRay)
        {
            _savedUiRayMask = uiRay.raycastMask;
            uiRay.raycastMask = uiOnlyRaycastMask; // prevents grabbing/teleporting through the menu
        }
    }

    // Hook this to the Start button's OnClick
    public void OnPressStart()
    {
        if (_started) return;
        _started = true;

        // hide menu
        if (startMenuCanvas) startMenuCanvas.SetActive(false);

        // restore UI ray mask
        if (uiRay) uiRay.raycastMask = gameplayRaycastMask;

        // re-enable gameplay interactors and locomotion
        foreach (var inter in gameplayInteractors) if (inter) inter.enabled = true;
        foreach (var b in locomotionProviders) if (b) b.enabled = true;
    }
}
