#include "SpeedRater.h"

SpeedRater::SpeedRater(fpos_t startPosition)
{
	this->startPosition = startPosition;
	Start();
}

double SpeedRater::GetSpeed(fpos_t progress)
{
	auto now = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(now - this->beginTime).count();
	return double((progress - this->startPosition) / duration) / 1000;
}

void SpeedRater::Start()
{
	this->beginTime = high_resolution_clock::now();
}
