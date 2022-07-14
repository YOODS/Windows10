#pragma once

class Event
{
	HANDLE hEvent_;
public:
	Event(BOOL bManualReset=TRUE);
	~Event();
	//0:normal,1:timeout,else:error
	INT wait(DWORD dwMilliseconds = INFINITE);
	BOOL waiting();
	BOOL wake(BOOL force = FALSE);
	BOOL reset(){ return ResetEvent(hEvent_); }
};

