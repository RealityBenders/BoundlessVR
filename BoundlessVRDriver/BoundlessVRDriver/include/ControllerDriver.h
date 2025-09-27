#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <openvr_driver.h>
#include <memory>
#include <Eigen/Dense>

#include "comms/MinBiTCore.h"
#include "comms/MinBiTTcpServer.h"

using namespace vr;

using Request = std::shared_ptr<MinBiTCore::Request>;

struct EigenPose {
    Eigen::Quaternionf rotation;
    Eigen::Vector3f position;
};

namespace DriverHeaders {
	//Headers from client (microcontroller) to server (driver)

	//Pings server to check connection
	#define PING 0x1 // 0 bytes, 0 byte response
	// Sends IMU data
	#define IMU_DATA 0x2 // followed by 12 floats (48 bytes)

	//Headers from server (driver) to client (microcontroller)

	//Acknowledge   
	#define ACK 0x1 // followed by response data
	//No acknowledge
	#define NACK 0x2 // 0 bytes
}

/**
This class controls the behavior of the controller. This is where you 
tell OpenVR what your controller has (buttons, joystick, trackpad, etc.).
This is also where you inform OpenVR when the state of your controller 
changes (for example, a button is pressed).

For the methods, take a look at the comment blocks for the ITrackedDeviceServerDriver 
class too. Those comment blocks have some good information.

This example driver will simulate a controller that has a joystick and trackpad on it.
It is hardcoded to just return a value for the joystick and trackpad. It will cause 
the game character to move forward constantly.
**/
class ControllerDriver : public ITrackedDeviceServerDriver
{
public:

	/**
	Initialize your controller here. Give OpenVR information 
	about your controller and set up handles to inform OpenVR when 
	the controller state changes.
	**/
	EVRInitError Activate(uint32_t unObjectId);

	/**
	Un-initialize your controller here.
	**/
	void Deactivate();

	/**
	Tell your hardware to go into stand-by mode (low-power).
	**/
	void EnterStandby();

	/**
	Take a look at the comment block for this method on ITrackedDeviceServerDriver. So as far 
	as I understand, driver classes like this one can implement lots of functionality that 
	can be categorized into components. This class just acts as an input device, so it will 
	return the IVRDriverInput class, but it could return other component classes if it had 
	more functionality, such as maybe overlays or UI functionality.
	**/
	void* GetComponent(const char* pchComponentNameAndVersion);

	/**
	Refer to ITrackedDeviceServerDriver. I think it sums up what this does well.
	**/
	void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize);

	/**
	Returns the Pose for your device. Pose is an object that contains the position, rotation, velocity, 
	and angular velocity of your device.
	**/
	DriverPose_t GetPose();

	/**
	You can retrieve the state of your device here and update OpenVR if anything has changed. This 
	method should be called every frame.
	**/
	void RunFrame();

	EigenPose GetHeadsetPose();

    Eigen::Vector3d ToEigen(const HmdVector3_t& v);
    HmdVector3_t ToOpenVR(const Eigen::Vector3d& v);
    Eigen::Quaterniond ToEigen(const HmdQuaternion_t& q);
    HmdQuaternion_t ToOpenVR(const Eigen::Quaterniond& q);

private:
    uint32_t driverId;

    VRInputComponentHandle_t joystickYHandle;
    VRInputComponentHandle_t trackpadYHandle;
    VRInputComponentHandle_t joystickXHandle;
    VRInputComponentHandle_t trackpadXHandle;

    std::vector<std::shared_ptr<MinBiTCore>> protocols; // Changed from single protocol to vector
    MinBiTTcpServer tcpServer{"BoundlessVR Controller", 8080};

    // Locomotion algorithm parameters for virtual joystick
    double joystickRadius = 0.3; // Radius of the virtual joystick in meters
    double joystickDeadzone = 0.05; // Deadzone to prevent drift
    double maxSpeed = 2.0; // Maximum speed in meters per second
    // Joystick center position
    Eigen::Vector2d joystickCenter = { 0.0, 0.0 };

    HmdQuaternion_t ExtractQuaternionFromPose(HmdMatrix34_t matrix);
    HmdVector3_t ExtractPositionFromPose(HmdMatrix34_t matrix);
    void firmwareReadHandler(std::shared_ptr<MinBiTCore> protocol, Request request);
};