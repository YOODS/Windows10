#pragma once

enum {
	EVENT_KEY,
	EVENT_IPC,
	EVENT_SELF,
	EVENTNUM						//���̓C�x���g��
};

class EventMulti;
class Event;
class IIpcServer;

extern char key[256];
extern EventMulti *events;
extern Event *eventSelf;
extern IIpcServer *ipc;
