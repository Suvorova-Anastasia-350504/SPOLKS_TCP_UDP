#pragma once

#include <chrono>

using namespace std::chrono;

class SpeedRater
{	
    high_resolution_clock::time_point beginTime;
	long long startPosition;
	
	void Start();
public:
	SpeedRater(long long startPosition);
	double GetSpeed(long long progress);
};

