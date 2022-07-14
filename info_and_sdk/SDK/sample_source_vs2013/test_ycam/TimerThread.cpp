#include <iostream>
#include <time.h>
#include <cassert>
#include <thread>
#include <deque>
#include "Elapse.h"
#include "WaitTimer.h"
#include "TimerThread.h"

using namespace std;

TimerThread::TimerThread() : worker_(0), hStop_(INVALID_HANDLE_VALUE), interval_(0)
{
	lastInterval_ = 0;
}

TimerThread::~TimerThread()
{
	stop();
}

void TimerThread::setInterval(int ms)
{
	interval_ = ms;
}

void TimerThread::start()
{
	pre_start();
	hStop_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	worker_ = new thread(work, this);
}

void TimerThread::stop()
{
	if (worker_){
		SetEvent(hStop_);
		worker_->join();
		delete worker_; worker_ = 0;
		CloseHandle(hStop_);
		hStop_ = INVALID_HANDLE_VALUE;
		clock_bin.clear();
	}
}

void TimerThread::work(TimerThread *a)
{
	Elapse e;
	WaitTimer w;
	double sleep_t = 0;			//累積待ち時間
	long run_counter = 0x7fffffff;	//実行カウンター
	double t0;						//初回実行時刻

	for (bool next = true; next && WaitForSingleObject(a->hStop_, 0) != WAIT_OBJECT_0;) {
		//カウンターリセット
		if (run_counter > 0x7ffffff0) {
			run_counter = 0;
			t0 = e.clock();
		}

		next=a->run();

		++run_counter;

		//時間待ち
		sleep_t += ((t0 + run_counter * a->interval_) - e.clock());
		if (sleep_t > 0) {
			w.sleep((int)(sleep_t*1e3+0.5));	//us
			sleep_t = 0;
		}
		else {
			w.sleep(1000);
			sleep_t -= 1.0;
		}
		{
			clock_t t = clock();
			a->clock_bin.push_back(t);
			if (a->clock_bin.size() > 2) {
				a->clock_bin.pop_front();
				long d = a->clock_bin.back() - a->clock_bin.front();
				a->lastInterval_ = (int)d;
				//printf("\r%.2ffps", (double)CLOCKS_PER_SEC/d);
			}
		}
	}
}

double TimerThread::cps()
{
	if (lastInterval_ == 0) return 0.0;
	return (double)CLOCKS_PER_SEC / lastInterval_;
}
