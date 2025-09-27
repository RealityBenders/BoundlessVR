using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactables;
using TMPro;  // only if using TextMeshPro


[RequireComponent(typeof(XRGrabInteractable))]
public class GrabbableTrigger : MonoBehaviour
{
    private XRGrabInteractable grabInteractable;

    [SerializeField] private GameObject uiPanel; // assign in Inspector
    [SerializeField] private TMP_Text uiText;    // optional: assign TMP text field

    private void Awake()
    {
        grabInteractable = GetComponent<XRGrabInteractable>();
    }

    private void OnEnable()
    {
        grabInteractable.selectEntered.AddListener(OnGrab);
    }

    private void OnDisable()
    {
        grabInteractable.selectEntered.RemoveListener(OnGrab);
    }

    private void OnGrab(SelectEnterEventArgs args)
    {
        ScoreManager.Instance.AddPoint();
        // Show UI panel
        if (uiPanel != null)
            uiPanel.SetActive(true);

        // Update UI text
        if (uiText != null)
            uiText.text = "You grabbed: " + gameObject.name;



    }
}
