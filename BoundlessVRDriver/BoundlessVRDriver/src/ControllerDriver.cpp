#include "ControllerDriver.h"

using namespace DriverHeaders;

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
        EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //Why VRScalarUnits_NormalizedTwoSided? Take a look at the comments on VRScalarUnits.
    
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
    //    ButtonMaskFromId(k_EButton_IndexController_JoyStick);
    //VRProperties()->SetUint64Property(props, Prop_SupportedButtons_Uint64, availableButtons);
    // Use a lambda to bind the member function
    // In ControllerDriver.cpp
    tcpServer.setInitHandler([](std::shared_ptr<MinBiTCore> protocol) {
        // Load packet lengths from JSON file
        protocol->loadIncomingByRequest(&incomingByRequest);
    });
    
    tcpServer.setReadHandler([this](std::shared_ptr<MinBiTCore> protocol, std::shared_ptr<MinBiTCore::Request> request) {
        this->firmwareReadHandler(protocol, request);
    });

    if(!tcpServer.begin()){
        VRDriverLog()->Log("ControllerDriver: Failed to start TCP server.");
        return VRInitError_Driver_Failed;
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

void ControllerDriver::RunFrame()
{
    // Gets stride speed estimate from imu data
    double strideSpeed = 0.0; // Placeholder for actual speed calculation

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

    Eigen::Vector2d joystickOutput = diff * (strideSpeed / maxSpeed);

    // Update all protocol instances if needed (example: send joystick data)
    for (auto& protocol : protocols) {
        // Example: protocol->writeVector2d(joystickOutput); // if such a method exists
        // protocol->sendAll();
    }

    VRDriverInput()->UpdateScalarComponent(joystickYHandle, joystickOutput.y(), 0);
    VRDriverInput()->UpdateScalarComponent(trackpadYHandle, joystickOutput.y(), 0);
    VRDriverInput()->UpdateScalarComponent(joystickXHandle, joystickOutput.x(), 0);
    VRDriverInput()->UpdateScalarComponent(trackpadXHandle, joystickOutput.x(), 0);
}

void ControllerDriver::Deactivate()
{
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

// Add this method to access headset pose through driver context
EigenPose ControllerDriver::GetHeadsetPose()
{
    Eigen::Quaterniond headsetRotation(1, 0, 0, 0); // Identity quaternion
    Eigen::Vector3d headsetPosition(0, 0, 0);

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
                    // Extract rotation from the pose (this would need proper implementation)
                    headsetRotation = ToEigen(ExtractQuaternionFromPose(trackedDevicePose.mDeviceToAbsoluteTracking));
                    headsetPosition = ToEigen(ExtractPositionFromPose(trackedDevicePose.mDeviceToAbsoluteTracking));
                    break;
                }
            }
        }
    }

    return EigenPose{headsetRotation, headsetPosition};
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

void ControllerDriver::firmwareReadHandler(std::shared_ptr<MinBiTCore> protocol, Request request) {
    // Ensures request did not time out
    
    // Gets response header
    uint8_t response = request->GetResponseHeader();
    //Reads serial packets
    switch (request->GetHeader()) {
        case PING: {
			VRDriverLog()->Log("Ping received.");
            // Sends acknowledge
            protocol->writeByte(ACK);
            protocol->sendAll();
            break;
        }
        case IMU_QUAT: {
            // Send acknowledge
            protocol->writeByte(ACK);
            protocol->sendAll();
            // Process IMU data
            // Reads in quaterniond
            Eigen::Quaterniond newIMUQuat = protocol->readQuaterniond();
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                imuQuat = newIMUQuat;
            }
            break;
        }
        case IMU_STEP: {
            // Send acknowledge
            protocol->writeByte(ACK);
            protocol->sendAll();
            // Process IMU step data
            // Reads in step timestamp
            uint64_t stepTime = protocol->readData<uint64_t>();
            VRDriverLog()->Log(("Step: " + std::to_string(stepTime)).c_str());
			break;
        }
        default:
            break;
    }
}