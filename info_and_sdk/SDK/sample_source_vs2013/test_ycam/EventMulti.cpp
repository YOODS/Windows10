#include <Windows.h>
#include "EventMulti.h"


EventMulti::EventMulti(int num, BOOL bManualReset)
{
	for (int i = 0; i < num; ++i){
		hEvents_.push_back(CreateEvent(NULL, bManualReset, FALSE, NULL));
	}
}


EventMulti::~EventMulti()
{
	for (auto i : hEvents_){
		CloseHandle(i);
	}
}

INT EventMulti::wait(DWORD dwMilliseconds)
{
	DWORD dwEventNo = WaitForMultipleObjects((DWORD)hEvents_.size(), &hEvents_[0], FALSE, dwMilliseconds);
	if (dwEventNo == WAIT_TIMEOUT) return -1;
	return dwEventNo - WAIT_OBJECT_0;
}

#define ARG_CHECK 	if (no < 0 || hEvents_.size() <= no){return FALSE;}

BOOL EventMulti::wake(int no, BOOL force)
{
	ARG_CHECK;
	if (force || waiting()){
		return SetEvent(hEvents_[no]);
	}
	return TRUE;
}


BOOL EventMulti::reset(int no){
	ARG_CHECK;
	return ResetEvent(hEvents_[no]);
}

BOOL EventMulti::waiting(DWORD  dwMilliseconds)
{
	return WAIT_TIMEOUT == WaitForMultipleObjects((DWORD)hEvents_.size(), &hEvents_[0], FALSE, dwMilliseconds);
}
