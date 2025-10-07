/**
 * Timer.h - Timer class for measuring elapsed time
 * Created by Carson G. Ray
*/

#ifndef Timer_h
#define Timer_h

//External imports
#include <chrono>

using namespace std::chrono;

class Timer {
public:
	//Constructor
	Timer();
	//Resets timer
	void reset();
	//Returns elapsed time in microseconds
	uint64_t elapsedMicros();
	//Returns elapsed time in milliseconds
	uint64_t elapsedMillis();
	//Returns elapsed time in seconds
	double elapsedSeconds();
private:
	//Start timepoint
	high_resolution_clock::time_point startTime;
};


#endif