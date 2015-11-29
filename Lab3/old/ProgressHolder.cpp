#include "ProgressHolder.h"


ProgressHolder::ProgressHolder(fpos_t startPos, fpos_t endPos, string filename)
				: startPos(startPos), lastPos(startPos), endPos(endPos), filename(filename) {
	this->nextLogPos = startPos + endPos / 100 * LOG_INTERVAL_PERCENT;
	this->startTime = high_resolution_clock::now();
	this->lastTime = this->startTime;
	cout << filename << " "<< 0 << "%;" << endl;
}


ProgressHolder::~ProgressHolder() {
}

void ProgressHolder::log(fpos_t currentPos) {
	if (currentPos >= nextLogPos) {
		//log and calc next log pos
		auto now = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(now - this->lastTime).count();
		double speed = (currentPos - lastPos) / (double)duration;

		lastPos = currentPos;
		lastTime = now;

		double progress = (double)currentPos / (double)endPos * 100.0;
		cout << filename << " " << progress << " %\t" << speed << " MB/s" << endl;


		nextLogPos = endPos / 100 * LOG_INTERVAL_PERCENT + nextLogPos;
		cout << "NEXT LOG POS  : " << nextLogPos << endl;
		if (nextLogPos > endPos) {
			nextLogPos = endPos;
		}
	}
}

void ProgressHolder::logFinish() {
	auto now = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(now - this->startTime).count();
	double speed = (endPos - startPos) / (double)duration;

	cout << filename << " 100.0" << " %\t Average speed: " << speed << " MB/s" << endl;
}