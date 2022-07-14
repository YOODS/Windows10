#pragma once
#include <memory>
#include <thread>
#include <vector>

class EventMulti;
class WaitTimer;

class SelfProc
{
	EventMulti *target_;
	std::unique_ptr<EventMulti> evt_;
	std::thread thd_proc_;
	std::vector<std::string> cmds_;
	std::unique_ptr<WaitTimer> tmr_;
	//
	static void event_loop(SelfProc *);
public:
	SelfProc(EventMulti *target);
	~SelfProc();
	void add(LPCTSTR cmd, ...);
	void add(LPCTSTR cmd, va_list args);
	void clear() { cmds_.clear(); }
	bool exec();
	bool exec(LPCSTR cmd, ...);
	bool busy();
	bool empty() { return cmds_.empty(); }
};

