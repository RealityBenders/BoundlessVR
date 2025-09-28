using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadGame : MonoBehaviour
{
    public void StartButton()
    {
        Debug.Log("starting main scene");
        SceneManager.LoadScene("MainGame");
        // replace "DemoScene" with the exact name of your scene
    }
}
