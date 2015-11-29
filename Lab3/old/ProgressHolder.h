#pragma once

#include <chrono>
#include "DataTypeDefinitions.h"
#include "utils.h"
#include <math.h>

#define LOG_INTERVAL_PERCENT 10

using namespace std::chrono;

class ProgressHolder
{
	high_resolution_clock::time_point startTime;
	high_resolution_clock::time_point lastTime;
	fpos_t startPos;
	fpos_t lastPos;
	string filename;

	//file size
	fpos_t endPos;

	//If currentPos > nextLogPos => need log.
	fpos_t nextLogPos;
	
	void start();
public:
	ProgressHolder(fpos_t startPos, fpos_t endPos, string filename);
	~ProgressHolder();

	void log(fpos_t currentPos);
	void logFinish();
};

