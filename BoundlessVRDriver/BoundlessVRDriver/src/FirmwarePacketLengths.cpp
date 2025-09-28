#include "FirmwarePacketLengths.h"

// Definitions of packet length maps

// Incoming packet lengths by request header
std::unordered_map<uint8_t, int16_t> incomingByRequest = {
    { 1,  0 },   // PING: 0 bytes
    { 2,  16 },  // IMU_QUAT: 16 bytes (4 floats)
    { 3,  8 },   // IMU_STEP: 8 bytes (1 uint64_t)
};