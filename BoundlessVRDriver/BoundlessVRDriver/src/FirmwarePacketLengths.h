#ifndef FIRMWARE_PACKET_LENGTHS
#define FIRMWARE_PACKET_LENGTHS

#include <unordered_map>
#include <cstdint>

// External declarations - definitions will be in packet_lengths.cpp
extern std::unordered_map<uint8_t, int16_t> incomingByRequest;

#endif