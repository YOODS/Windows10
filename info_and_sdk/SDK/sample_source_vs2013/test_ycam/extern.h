#pragma once

enum {
	EVENT_KEY,
	EVENT_IPC,
	EVENT_SELF,
	EVENTNUM						//入力イベント数
};

class EventMulti;
class Event;
class IIpcServer;

extern char key[256];
extern EventMulti *events;
extern Event *eventSelf;
extern IIpcServer *ipc;
