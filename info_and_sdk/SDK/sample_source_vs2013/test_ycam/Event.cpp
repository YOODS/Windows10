#include <Windows.h>
#include "Event.h"


Event::Event(BOOL bManualReset)
{
	hEvent_ = CreateEvent(NULL, bManualReset, FALSE, NULL);
}


Event::~Event()
{
	CloseHandle(hEvent_);
}

INT Event::wait(DWORD dwMilliseconds)
{
	DWORD ret = WaitForSingleObject(hEvent_, dwMilliseconds);
	if (ret==WAIT_OBJECT_0)return 0;
	else if (ret == WAIT_TIMEOUT)return 1;
	return -1;
}

BOOL Event::wake(BOOL force)
{
	if (force || waiting()){
		return SetEvent(hEvent_);
	}
	return TRUE;
}

BOOL Event::waiting()
{
	return WAIT_TIMEOUT == WaitForSingleObject(hEvent_, 0);
}

