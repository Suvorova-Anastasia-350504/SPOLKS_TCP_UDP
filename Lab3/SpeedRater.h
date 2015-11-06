#pragma once

#include <chrono>

using namespace std::chrono;

class SpeedRater
{	
	time_point<steady_clock> beginTime;
	long long startPosition;
	
	void Start();
public:
	SpeedRater(long long startPosition);
	double GetSpeed(long long progress);
};

