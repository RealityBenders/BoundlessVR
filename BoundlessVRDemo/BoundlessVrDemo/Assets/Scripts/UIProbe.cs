using UnityEngine;
using UnityEngine.EventSystems;

public class UIProbe : MonoBehaviour, IPointerEnterHandler, IPointerDownHandler, IPointerUpHandler, IPointerClickHandler
{
    public void OnPointerEnter(PointerEventData eventData) => Debug.Log("UIProbe: Hover " + gameObject.name);
    public void OnPointerDown(PointerEventData eventData) => Debug.Log("UIProbe: PointerDown " + gameObject.name);
    public void OnPointerUp(PointerEventData eventData) => Debug.Log("UIProbe: PointerUp " + gameObject.name);
    public void OnPointerClick(PointerEventData eventData) => Debug.Log("UIProbe: PointerClick " + gameObject.name);
}
