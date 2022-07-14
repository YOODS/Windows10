#pragma once

#include "IComIpc.h"

class CComIpc
{
	HMODULE hLib_;
public:
	CComIpc();
	~CComIpc();
	bool loadLib();
	void freeLib();
	IIpcServer *createIpcServer();
	IIpcClient *createIpcClient();
};

