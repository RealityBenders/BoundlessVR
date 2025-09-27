using UnityEngine;
using Unity.XR.CoreUtils;        // for XROrigin (if you want to detect by component)


public class ObstacleHitTrigger : MonoBehaviour
{
    [SerializeField] private string playerTag = "Player"; // or leave blank to use component check
    [SerializeField] private GameObject uiPanel; // assign in Inspector
    [SerializeField] private GameOverFreeze freezer; // assign in Inspector




    private void OnTriggerEnter(Collider other)
    {
        // Option A: tag check
        if (!string.IsNullOrEmpty(playerTag) && other.CompareTag(playerTag))
        {
            ObstacleHitPlayer();
            return;
        }

        // Option B: component check (works even if child collider hits)
        if (other.GetComponentInParent<XROrigin>() != null ||
            other.GetComponentInParent<CharacterController>() != null)
        {
            ObstacleHitPlayer();
        }
    }

    private void ObstacleHitPlayer()
    {
        Debug.Log("Player bumped into trigger zone!");
        if (uiPanel != null)
            uiPanel.SetActive(true);
        if (freezer)
            freezer.FreezeForGameOver();
    }

    // // Update is called once per frame
    // void Update()
    // {

    // }
}
