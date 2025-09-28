#include <ControllerDriver.h>


EVRInitError ControllerDriver::Activate(uint32_t unObjectId)
{
	driverId = unObjectId; //unique ID for your driver

	PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(driverId); //this gets a container object where you store all the information about your driver

	VRProperties()->SetStringProperty(props, Prop_InputProfilePath_String, "{example}/input/controller_profile.json"); //tell OpenVR where to get your driver's Input Profile
	VRProperties()->SetInt32Property(props, Prop_ControllerRoleHint_Int32, ETrackedControllerRole::TrackedControllerRole_Treadmill); //tells OpenVR what kind of device this is
	VRDriverInput()->CreateScalarComponent(props, "/input/joystick/y", &joystickYHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //sets up handler you'll use to send joystick commands to OpenVR with, in the Y direction (forward/backward)
	VRDriverInput()->CreateScalarComponent(props, "/input/trackpad/y", &trackpadYHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //sets up handler you'll use to send trackpad commands to OpenVR with, in the Y direction
	VRDriverInput()->CreateScalarComponent(props, "/input/joystick/x", &joystickXHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //Why VRScalarType_Absolute? Take a look at the comments on EVRScalarType.
	VRDriverInput()->CreateScalarComponent(props, "/input/trackpad/x", &trackpadXHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //Why VRScalarUnits_NormalizedTwoSided? Take a look at the comments on EVRScalarUnits.
	
	//The following properites are ones I tried out because I saw them in other samples, but I found they were not needed to get the sample working.
	//There are many samples, take a look at the openvr_header.h file. You can try them out.

	//VRProperties()->SetUint64Property(props, Prop_CurrentUniverseId_Uint64, 2);
	//VRProperties()->SetBoolProperty(props, Prop_HasControllerComponent_Bool, true);
	//VRProperties()->SetBoolProperty(props, Prop_NeverTracked_Bool, true);
	//VRProperties()->SetInt32Property(props, Prop_Axis0Type_Int32, k_eControllerAxis_TrackPad);
	//VRProperties()->SetInt32Property(props, Prop_Axis2Type_Int32, k_eControllerAxis_Joystick);
	//VRProperties()->SetStringProperty(props, Prop_SerialNumber_String, "example_controler_serial");
	//VRProperties()->SetStringProperty(props, Prop_RenderModelName_String, "vr_controller_vive_1_5");
	//uint64_t availableButtons = ButtonMaskFromId(k_EButton_SteamVR_Touchpad) |
	//	ButtonMaskFromId(k_EButton_IndexController_JoyStick);
	//VRProperties()->SetUint64Property(props, Prop_SupportedButtons_Uint64, availableButtons);

	WSADATA wsaData;
	int wsaerr;
	WORD wVersionRequested = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequested, &wsaData);
	//WSAStartup resturns 0 if it is successfull or non zero if failed
	if (wsaerr != 0) {
		VRDriverLog()->Log("The Winsock dll not found!");
		return VRInitError_Driver_Unknown;
	}
	else {
		VRDriverLog()->Log("The Winsock dll found");
	}

	/*
	refer ServerCreation.md
	*/
	SOCKET serverSocket;
	serverSocket = INVALID_SOCKET; //initializing as a inivalid socket
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//check if creating socket is successfull or not
	if (serverSocket == INVALID_SOCKET) {
		WSACleanup();
		return VRInitError_Driver_Unknown;
	}
	else {
		VRDriverLog()->Log("socket is OK!");
	}

	/*
	3. Bind the socket to ip address and port number
	//refer ServerCreation.md
	*/
	sockaddr_in service; //initialising service as sockaddr_in structure
	service.sin_family = AF_INET;
	//InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr); 
	//InetPton function is encountering an issue, so replaced with the following line which uses inet_addr to convert IP address string to the binary form (only for ipv4) and storing it
	const char* ip_address_str = "127.0.0.1";
	service.sin_addr.s_addr = inet_pton(AF_INET, ip_address_str, &(service.sin_addr));
	//    service.sin_addr.s_addr = inet_addr("192.168.43.42");
	service.sin_port = htons(8080);
	//using the bind function
	if (::bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		closesocket(serverSocket);
		WSACleanup();
		return VRInitError_Driver_Unknown;
	}
	else {
		VRDriverLog()->Log("bind() is OK!");
	}

	//4. Listen to incomming connections
	if (listen(serverSocket, 1) == SOCKET_ERROR) {
		VRDriverLog()->Log("listen(): Error listening on socket");
	}
	else {
		VRDriverLog()->Log("listen() is OK!, I'm waiting for new connections...");
	}	

	return VRInitError_None;
}

DriverPose_t ControllerDriver::GetPose()
{
	DriverPose_t pose = { 0 }; //This example doesn't use Pose, so this method is just returning a default Pose.
	pose.poseIsValid = false;
	pose.result = TrackingResult_Calibrating_OutOfRange;
	pose.deviceIsConnected = true;

	HmdQuaternion_t quat;
	quat.w = 1;
	quat.x = 0;
	quat.y = 0;
	quat.z = 0;

	pose.qWorldFromDriverRotation = quat;
	pose.qDriverFromHeadRotation = quat;

	return pose;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = s.find(delimiter);

	while (end != std::string::npos) {
		tokens.push_back(s.substr(start, end - start));
		start = end + delimiter.length();
		end = s.find(delimiter, start);
	}
	tokens.push_back(s.substr(start)); // Add the last token

	return tokens;
}

void ControllerDriver::RunFrame()
{
	VRDriverLog()->Log("ControllerDriver::RunFrame() called.");

	////Since we used VRScalarUnits_NormalizedTwoSided as the unit, the range is -1 to 1.
	//VRDriverInput()->UpdateScalarComponent(joystickYHandle, 0.95f, 0); //move forward
	//VRDriverInput()->UpdateScalarComponent(trackpadYHandle, 0.95f, 0); //move foward
	//VRDriverInput()->UpdateScalarComponent(joystickXHandle, 0.0f, 0); //change the value to move sideways
	//VRDriverInput()->UpdateScalarComponent(trackpadXHandle, 0.0f, 0); //change the value to move sideways

	SOCKET acceptSocket;
	std::string data;
	acceptSocket = accept(serverSocket, NULL, NULL);
	if (acceptSocket == INVALID_SOCKET) {
		VRDriverLog()->Log("accept failed");
		WSACleanup();
	}
	else {
		VRDriverLog()->Log("accept() is OK!");
		char buffer[1024] = { 0 };
		int bytes_received = recv(acceptSocket, buffer, sizeof(buffer), 0);
		if (bytes_received > 0) {
			std::string receivedData(buffer);
			data = receivedData;
		}
		VRDriverLog()->Log(("Received data: %s", buffer));
		closesocket(acceptSocket);
	}

	if (data.substr(0, 2) == "STP") {
		std::string delimiter = ",";
		std::vector<std::string> dataVector = split(data, delimiter);
		int currStep = std::stoi(dataVector[1]);
		unsigned long long time = std::stoull(dataVector[2]);
		double newStrideSpeed = strideLength / (((time - imuStepTime) / (currStep - stepCount)) / 1000000.0); // imuStepTime is in microseconds
		stepCount = currStep;
		imuStepTime = time;
		// Interpolate speed to avoid sudden jumps
		strideSpeed = strideSpeed * (1.0 - strideSpeedSmoothing) + newStrideSpeed * strideSpeedSmoothing;
		// Cap to max speed
		strideSpeed = min(strideSpeed, maxSpeed);
	}
	else if (data.substr(0, 2) == "ROT") {
		std::string delimiter = ",";
		std::vector<std::string> dataVector = split(data, delimiter);
		Eigen::Quaterniond newIMUQuat = Eigen::Quaterniond(
			std::stod(dataVector[1]),
			std::stod(dataVector[2]),
			std::stod(dataVector[3]),
			std::stod(dataVector[4])
		);
		// Get headset position (3d)
		EigenPose headsetPose = GetHeadsetPose();
		Eigen::Vector3d headsetPos = headsetPose.position;
		// Get headset position (2d)
		Eigen::Vector2d pos2d = Eigen::Vector2d(headsetPos.x(), headsetPos.z());

		// Gets difference from joystick center
		Eigen::Vector2d diff = pos2d - joystickCenter;
		if (diff.norm() > joystickRadius) {
			// If outside joystick, move joystick center so that headset position is on the perimeter
			joystickCenter += diff.normalized() * joystickRadius;
		}
		else {
			// If inside deadzone, set difference to zero
			if (diff.norm() < joystickDeadzone) {
				diff = Eigen::Vector2d::Zero();
			}
		}


	}
}

void ControllerDriver::Deactivate()
{
	WSACleanup();
	driverId = k_unTrackedDeviceIndexInvalid;
}

void* ControllerDriver::GetComponent(const char* pchComponentNameAndVersion)
{
	//I found that if this method just returns null always, it works fine. But I'm leaving the if statement in since it doesn't hurt.
	//Check out the IVRDriverInput_Version declaration in openvr_driver.h. You can search that file for other _Version declarations 
	//to see other components that are available. You could also put a log in this class and output the value passed into this 
	//method to see what OpenVR is looking for.
	if (strcmp(IVRDriverInput_Version, pchComponentNameAndVersion) == 0)
	{
		return this;
	}
	return NULL;
}

void ControllerDriver::EnterStandby() {}

void ControllerDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) 
{
	if (unResponseBufferSize >= 1)
	{
		pchResponseBuffer[0] = 0;
	}
}

// Conversion functions between OpenVR and Eigen types
Eigen::Vector3d ControllerDriver::ToEigen(const HmdVector3_t& v) {
	return Eigen::Vector3d(v.v[0], v.v[1], v.v[2]);
}

HmdVector3_t ControllerDriver::ToOpenVR(const Eigen::Vector3d& v) {
	HmdVector3_t out;
	out.v[0] = v.x();
	out.v[1] = v.y();
	out.v[2] = v.z();
	return out;
}

Eigen::Quaterniond ControllerDriver::ToEigen(const HmdQuaternion_t& q) {
	return Eigen::Quaterniond(q.w, q.x, q.y, q.z);
}

HmdQuaternion_t ControllerDriver::ToOpenVR(const Eigen::Quaterniond& q) {
	HmdQuaternion_t out;
	out.w = q.w();
	out.x = q.x();
	out.y = q.y();
	out.z = q.z();

	return out;
}

// Add this method to access headset pose through driver context
EigenPose ControllerDriver::GetHeadsetPose()
{
	Eigen::Quaterniond headsetRotation(1, 0, 0, 0); // Identity quaternion
	Eigen::Vector3d headsetPosition(0, 0, 0);
	Eigen::Vector3d headsetVelocity(0, 0, 0);

	// Access through VRServerDriverHost
	if (VRServerDriverHost())
	{
		// Look for HMD device using VRProperties instead
		for (uint32_t deviceId = 0; deviceId < k_unMaxTrackedDeviceCount; ++deviceId)
		{
			// Get property container for this device
			PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(deviceId);
			if (props != k_ulInvalidPropertyContainer)
			{
				// Get device class using VRProperties
				ETrackedPropertyError propError;
				int32_t deviceClass = VRProperties()->GetInt32Property(props, Prop_DeviceClass_Int32, &propError);

				if (propError == TrackedProp_Success && deviceClass == TrackedDeviceClass_HMD)
				{
					// Get the pose from the server driver host
					TrackedDevicePose_t trackedDevicePose;
					VRServerDriverHost()->GetRawTrackedDevicePoses(0, &trackedDevicePose, 1);

					headsetRotation = ToEigen(ExtractQuaternionFromPose(trackedDevicePose.mDeviceToAbsoluteTracking));
					headsetPosition = ToEigen(ExtractPositionFromPose(trackedDevicePose.mDeviceToAbsoluteTracking));
					headsetVelocity = ToEigen(trackedDevicePose.vVelocity);
					break;
				}
			}
		}
	}

	return EigenPose{ headsetRotation, headsetPosition, headsetVelocity };
}

HmdQuaternion_t ControllerDriver::ExtractQuaternionFromPose(HmdMatrix34_t matrix)
{
	vr::HmdQuaternion_t q;

	q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
	q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
	q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);
	return q;
}

HmdVector3_t ControllerDriver::ExtractPositionFromPose(HmdMatrix34_t matrix)
{
	vr::HmdVector3_t p;
	p.v[0] = matrix.m[0][3];
	p.v[1] = matrix.m[1][3];
	p.v[2] = matrix.m[2][3];
	return p;
}