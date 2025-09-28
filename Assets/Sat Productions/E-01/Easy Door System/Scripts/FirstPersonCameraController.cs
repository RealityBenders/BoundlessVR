namespace EasyDoorSystem
{
    using UnityEngine;

    public class FirstPersonCameraController : MonoBehaviour
    {
        private float movementSpeed = 10f;
        private float fastMovementSpeed = 100f;
        private float freeLookSensitivity = 3f;
        private float zoomSensitivity = 10f;
        private float fastZoomSensitivity = 50f;

        private bool isLooking = false;

        void Update()
        {
            HandleMovement();
            HandleLook();
            HandleZoom();
        }

        void HandleMovement()
        {
            float speed = Input.GetKey(KeyCode.LeftShift) ? fastMovementSpeed : movementSpeed;

            Vector3 moveDirection = Vector3.zero;

            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow))
                moveDirection += transform.forward;
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow))
                moveDirection -= transform.forward;
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow))
                moveDirection += transform.right;
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow))
                moveDirection -= transform.right;
            if (Input.GetKey(KeyCode.Q) || Input.GetKey(KeyCode.PageDown))
                moveDirection -= transform.up;
            if (Input.GetKey(KeyCode.E) || Input.GetKey(KeyCode.PageUp))
                moveDirection += transform.up;

            if (moveDirection != Vector3.zero)
                transform.position += moveDirection.normalized * speed * Time.deltaTime;
        }

        void HandleLook()
        {
            if (Input.GetKeyDown(KeyCode.Mouse1))
                StartLooking();
            if (Input.GetKeyUp(KeyCode.Mouse1))
                StopLooking();

            if (isLooking)
            {
                float mouseX = Input.GetAxis("Mouse X") * freeLookSensitivity;
                float mouseY = -Input.GetAxis("Mouse Y") * freeLookSensitivity;

                Vector3 eulerAngles = transform.eulerAngles;
                eulerAngles.x += mouseY;
                eulerAngles.y += mouseX;
                transform.eulerAngles = eulerAngles;
            }
        }

        void HandleZoom()
        {
            float zoomAxis = Input.GetAxis("Mouse ScrollWheel");
            if (zoomAxis != 0)
            {
                float sensitivity = Input.GetKey(KeyCode.LeftShift) ? fastZoomSensitivity : zoomSensitivity;
                transform.position += transform.forward * zoomAxis * sensitivity;
            }
        }

        void StartLooking()
        {
            isLooking = true;
            Cursor.visible = false;
            Cursor.lockState = CursorLockMode.Locked;
        }

        void StopLooking()
        {
            isLooking = false;
            Cursor.visible = true;
            Cursor.lockState = CursorLockMode.None;
        }

        void OnDisable()
        {
            StopLooking();
        }
    }
}