/**
 * Timer.cpp - Timer class for measuring elapsed time
 * Created by Carson G. Ray
*/

#include "Timer.h"

using namespace std::chrono;

Timer::Timer() {
	startTime = high_resolution_clock::now();
}

void Timer::reset() {
	startTime = high_resolution_clock::now();
}

uint64_t Timer::elapsedMicros() {
	high_resolution_clock::time_point end = high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - startTime); // Use microseconds
	return duration.count();
}

uint64_t Timer::elapsedMillis() {
	high_resolution_clock::time_point end = high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime); // Use milliseconds
	return duration.count();
}

double Timer::elapsedSeconds() {
	return elapsedMicros() / 1000000.0;
}
