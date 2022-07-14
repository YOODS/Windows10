#include <Windows.h>
#include "Elapse.h"


LARGE_INTEGER Elapse::freq_;

Elapse::Elapse() : interval_(0.0)
{
	if (!freq_.QuadPart){
		QueryPerformanceFrequency(&freq_);
	}
	reset();
}


Elapse::~Elapse()
{
}

