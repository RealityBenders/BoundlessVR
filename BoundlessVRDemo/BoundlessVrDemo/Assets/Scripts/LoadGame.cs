using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadGame : MonoBehaviour
{
    public void StartButton()
    {
        Debug.Log("starting basic scene");
        SceneManager.LoadScene("BasicScene");
        // replace "DemoScene" with the exact name of your scene
    }
}
