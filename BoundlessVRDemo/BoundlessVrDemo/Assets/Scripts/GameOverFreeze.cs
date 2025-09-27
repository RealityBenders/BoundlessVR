using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.InputSystem;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Locomotion;
using UnityEngine.XR.Interaction.Toolkit.Locomotion.Movement;
using UnityEngine.XR.Interaction.Toolkit.Locomotion.Turning;
using UnityEngine.XR.Interaction.Toolkit.Locomotion.Teleportation;

public class GameOverFreeze : MonoBehaviour
{
    [Header("XR Providers to disable")]
    [SerializeField] private LocomotionMediator locomotionSystem;         // optional
    [SerializeField] private ContinuousMoveProvider moveProvider;   // e.g., ActionBasedContinuousMoveProvider
    [SerializeField] private ContinuousTurnProvider turnProvider;   // or SnapTurnProviderBase
    [SerializeField] private TeleportationProvider teleportProvider;    // if you use teleport

    [Header("Input System (optional but robust)")]
    [SerializeField] private InputActionAsset inputActions;             // your Input Actions asset
    [SerializeField]
    private string[] locomotionMapsToDisable =         // typical XR maps
        { "XRI LeftHand Locomotion", "XRI RightHand Locomotion", "XRI Head" };

    private bool isFrozen;

    public void FreezeForGameOver()
    {
        if (isFrozen) return;
        isFrozen = true;

        // Disable XR providers
        if (moveProvider)
        {
            moveProvider.enabled = false;
            Debug.Log("move provider disabled");
        }
        if (turnProvider) turnProvider.enabled = false;
        if (teleportProvider) teleportProvider.enabled = false;
        if (locomotionSystem) locomotionSystem.enabled = false; // optional; providers off is usually enough
        // Debug.Log("Everything is frozen");

        // Disable locomotion input maps (prevents any residual input)
        if (inputActions)
        {
            foreach (var mapName in locomotionMapsToDisable)
            {
                var map = inputActions.FindActionMap(mapName, throwIfNotFound: false);
                if (map != null && map.enabled) map.Disable();
            }
        }

        // Optional: also hide controller rays if you don't want UI interaction
        // (disable XRRayInteractor components or their action maps likewise)
    }

    public void Unfreeze() // if you ever need to resume
    {
        if (!isFrozen) return;
        isFrozen = false;

        if (locomotionSystem) locomotionSystem.enabled = true;
        if (moveProvider) moveProvider.enabled = true;
        if (turnProvider) turnProvider.enabled = true;
        if (teleportProvider) teleportProvider.enabled = true;

        if (inputActions)
        {
            foreach (var mapName in locomotionMapsToDisable)
            {
                var map = inputActions.FindActionMap(mapName, throwIfNotFound: false);
                if (map != null && !map.enabled) map.Enable();
            }
        }
    }

    // Call from a button to restart
    public void RestartScene()
    {
        // If you paused time elsewhere, restore it first
        if (Time.timeScale == 0f) Time.timeScale = 1f;
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
    }
}
