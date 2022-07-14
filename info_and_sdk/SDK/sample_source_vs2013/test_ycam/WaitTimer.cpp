#include "WaitTimer.h"
#include <stdio.h>
#include <tchar.h>


#define dbg(...) { 	TCHAR buf[128];  \
					_stprintf_s(buf, __VA_ARGS__); \
					OutputDebugString(buf); }

WaitTimer::WaitTimer()
{
	hTimer_ = CreateWaitableTimer(NULL, FALSE, NULL);	//auto reset
	hCancel_ = CreateEvent(NULL, FALSE, FALSE, NULL);	//auto reset
}


WaitTimer::~WaitTimer()
{
	SetEvent(hCancel_);
	CloseHandle(hTimer_);
	CloseHandle(hCancel_);
}

int WaitTimer::sleep(int usec)
{
	LARGE_INTEGER interval;
	interval.QuadPart = -10 * usec; /* unit:100nsec */
	if (!SetWaitableTimer(hTimer_, &interval, 0, NULL, NULL, FALSE)) {
		dbg(_T("%s(%d) error:%d\n"), __FILE__, __LINE__, GetLastError());
		return -1;
	}
	HANDLE hEventArray[2];
	DWORD  dwEventNo;

	hEventArray[0] = hCancel_;
	hEventArray[1] = hTimer_;
	dwEventNo = WaitForMultipleObjects(2, hEventArray, FALSE, INFINITE) - WAIT_OBJECT_0;
	if (dwEventNo == 0){
		dbg(_T("%s(%d) canceled.\n"), __FILE__, __LINE__);
		return 1;	//cancel
	}
	else if (dwEventNo == 1) return 0;	//timer
	dbg(_T("%s(%d) unknown error:%d\n"), __FILE__, __LINE__, GetLastError());
	return -2;
}

void WaitTimer::cancel()
{
	SetEvent(hCancel_);
}

