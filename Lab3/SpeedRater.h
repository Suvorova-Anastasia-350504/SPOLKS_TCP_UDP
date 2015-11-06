#pragma once
#include <chrono>

using namespace std::chrono;

class SpeedRater
{	
	time_point<steady_clock> beginTime;
	fpos_t startPosition;
	
	void Start();
public:
	SpeedRater(fpos_t startPosition);
	double GetSpeed(fpos_t progress);
};

