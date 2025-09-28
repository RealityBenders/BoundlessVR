using UnityEngine;
using Unity.XR.CoreUtils;        // for XROrigin (if you want to detect by component)

public class PlayerBumpTrigger : MonoBehaviour
{
    [SerializeField] private string playerTag = "Player"; // or leave blank to use component check
    [SerializeField] private Animator fallingObjectAnimator;
    [SerializeField] private string boolName = "fallTriggered"; // Bool parameter in Animator
    [SerializeField] private float delay = 2f;    // Seconds to wait after bump



    private void OnTriggerEnter(Collider other)
    {
        // Option A: tag check
        if (!string.IsNullOrEmpty(playerTag) && other.CompareTag(playerTag))
        {
            StartCoroutine(TriggerAnimationAfterDelay());
            return;
        }

        // Option B: component check (works even if child collider hits)
        if (other.GetComponentInParent<XROrigin>() != null ||
            other.GetComponentInParent<CharacterController>() != null)
        {
            StartCoroutine(TriggerAnimationAfterDelay());
        }
    }

    private System.Collections.IEnumerator TriggerAnimationAfterDelay()
    {
        Debug.Log("Player bumped into trigger zone!");
        yield return new WaitForSeconds(delay);
        Debug.Log("Object falling");
        fallingObjectAnimator.SetBool(boolName, true);
    }

    // private void HandlePlayerEntered()
    // {
    //     Debug.Log("Player bumped into trigger zone!");
    //     // TODO: call your UI, score, audio, etc.

    //     // e.g., popup.SetActive(true);
    // }

    private void OnTriggerExit(Collider other)
    {
        // Optional: handle exit
        // popup.SetActive(false);
    }
}
