#pragma once
#include <vector>

class EventMulti
{
	std::vector<HANDLE> hEvents_;
public:
	EventMulti(int num=1, BOOL bManualReset=TRUE);
	~EventMulti();
	//0<=normal,-1:timeout
	INT wait(DWORD dwMilliseconds = INFINITE);
	BOOL waiting(DWORD  dwMilliseconds = 0);
	BOOL wake(int no=0, BOOL force = FALSE);
	BOOL reset(int no=0);
};

