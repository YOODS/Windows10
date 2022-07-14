#pragma once
#include <Windows.h>
#include <thread>
#include <deque>

class Elapse;

class TimerThread
{
	std::deque<long> clock_bin;
protected:
	std::thread *worker_;
	HANDLE hStop_;
	int interval_;
	int lastInterval_;

	virtual bool run() = 0;
	virtual void pre_start(){}
	static void work(TimerThread *a);
public:
	TimerThread();
	virtual ~TimerThread();
	void setInterval(int ms);
	void start();
	void stop();
	bool isRunning(){ return worker_ ? true:false; }
	int interval(){ return interval_; }
	int lastInterval();
	double cps();	//counts per second
};

