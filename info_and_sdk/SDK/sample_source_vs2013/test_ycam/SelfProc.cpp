#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <tchar.h>

#include "EventMulti.h"
#include "Event.h"
#include "WaitTimer.h"
#include "extern.h"
#include "IComIpc.h"
#include "Elapse.h"
#include "SelfProc.h"

enum {
	EVENT_SELF_EXIT,	//終了
	EVENT_SELF_EXEC,	//実行
	EVENTSELFNUM		
};

using namespace std;

SelfProc::SelfProc(EventMulti *target):target_(target)
{
	evt_.reset(new EventMulti(EVENTSELFNUM));
	tmr_.reset(new WaitTimer);
	thd_proc_ = thread(event_loop, this);
}

SelfProc::~SelfProc()
{
	evt_->wake(EVENT_SELF_EXIT);
	if (thd_proc_.joinable()) thd_proc_.join();
}

void SelfProc::event_loop(SelfProc *a)
{
	for (bool loop = true; loop;) {
		DWORD  dwEventNo = a->evt_->wait();
		Elapse elapse;
		for (;;) {
			if (dwEventNo == EVENT_SELF_EXEC) {
				for (int n = 0; !a->target_->waiting() && n < 200; ++n) {
					a->tmr_->sleep(5000);	//us
				}
				//イベント待ちを確認
				if (!a->target_->waiting()) {
					fprintf(stderr,"error: timeout: target proc\n");
					break;
				}
				for (auto c : a->cmds_) {
					if (!a->target_->waiting()) {
						fprintf(stderr, "warning: target not waiting. so wait...\n");
						for (int n = 0; !a->target_->waiting() && n < 200; ++n) {
							a->tmr_->sleep(5000);	//us
						}
						if (!a->target_->waiting()) {
							fprintf(stderr, "error: timeout: target proc. break loop!\n");
							break;
						}
						fprintf(stderr, "->OK!\n");
					}
					strcpy_s(key, sizeof(key), c.c_str());
					fprintf(stderr, "%s", key);
					events->wake(EVENT_SELF);
					eventSelf->wait();	//終わるまで待つ
					eventSelf->reset();
					for (int n = 0; !events->waiting() && n < 10; ++n) {	//念の為イベント待ち状態になるまで待つ
						a->tmr_->sleep(1000);	//us
					}
					a->tmr_->sleep(1000);	//念の為(意味はないかも)
				}
			}
			else loop = false;
			break;
		}
		a->evt_->reset(dwEventNo);
		if (ipc){
			int ms = 200 - (int)elapse.query();
			if (0 < ms){
				a->tmr_->sleep(ms * 1000);	//1行のみとかだとclientの準備が間に合わないので
			}
			ipc->trigger(_T("END"));
		}
	}
}

void SelfProc::add(LPCTSTR cmd, ...)
{
	TCHAR buf[256];
	va_list args;
	va_start(args, cmd);
	_vsntprintf_s(buf, sizeof(buf), _TRUNCATE, cmd, args);
	va_end(args);
	cmds_.push_back(buf);
}

void SelfProc::add(LPCTSTR cmd, va_list args)
{
	TCHAR buf[256];
	_vsntprintf_s(buf, sizeof(buf), _TRUNCATE, cmd, args);
	cmds_.push_back(buf);
}

bool SelfProc::exec()
{
	return evt_->wake(EVENT_SELF_EXEC) ? true : false;
}

bool SelfProc::exec(LPCTSTR cmd, ...)
{
	clear();
	va_list args;
	va_start(args, cmd);
	add(cmd, args);
	va_end(args);
	return exec();
}

bool SelfProc::busy()
{
	return !evt_->waiting();
}