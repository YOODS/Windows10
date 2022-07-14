#pragma once
#include <Windows.h>

class WaitTimer
{
	HANDLE hTimer_;
	HANDLE hCancel_;
public:
	WaitTimer();
	~WaitTimer();
	int sleep(int usec);
	void cancel();
};

