#ifndef DRIVER_PACKET_LENGTHS
#define DRIVER_PACKET_LENGTHS

#include <Arduino.h>
#include <unordered_map>


std::unordered_map<uint8_t, int16_t> outgoingByRequest = {
    { 1,  0 },
    { 2,  0 },
    { 3, 0 },
};

#endif