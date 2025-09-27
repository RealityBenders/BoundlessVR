#ifndef DriverHeaders_h
#define DriverHeaders_h

namespace DriverHeaders {
	//Headers from client (microcontroller) to server (driver)

	//Pings server to check connection
	#define PING 0x1 // 0 bytes, 0 byte response
	// Sends IMU quaternion data
	#define IMU_QUAT 0x2 // followed by 16 bytes (4 floats)

	//Sends IMU step data
	#define IMU_STEP 0x3 // followed by 8 bytes (1 uint64_t)

	//Headers from server (driver) to client (microcontroller)

	//Acknowledge   
	#define ACK 0x1 // followed by response data
	//No acknowledge
	#define NACK 0x2 // 0 bytes
}

#endif // InterfaceHeaders_h