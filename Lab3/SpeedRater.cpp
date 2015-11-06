#include "SpeedRater.h"

SpeedRater::SpeedRater(long long startPosition)
{
	this->startPosition = startPosition;
	Start();
}

double SpeedRater::GetSpeed(long long progress)
{
	auto now = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(now - this->beginTime).count();
	return double(progress - this->startPosition) / duration;
}

void SpeedRater::Start()
{
	this->beginTime = high_resolution_clock::now();
}
