#pragma once
#include <Windows.h>

class Elapse
{
	static 	LARGE_INTEGER freq_;
	LARGE_INTEGER start_;
	double interval_;
public:
	Elapse();
	~Elapse();
	void reset();
	double query();
	double interval();
	double clock();
};

inline void Elapse::reset(){
	QueryPerformanceCounter(&start_);
}

inline double Elapse::query(){
	LARGE_INTEGER stop;
	QueryPerformanceCounter(&stop);
	interval_ = ((1000.0*(stop.QuadPart - start_.QuadPart)) / (double)freq_.QuadPart);
	return interval_;
}

inline double Elapse::interval(){
	return interval_;
}

inline double Elapse::clock(){
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (1000.0*now.QuadPart) / (double)freq_.QuadPart;
}
