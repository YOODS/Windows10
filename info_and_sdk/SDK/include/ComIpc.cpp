#include <Windows.h>
#include "ComIpc.h"

CComIpc::CComIpc() : hLib_(0)
{
}

CComIpc::~CComIpc()
{
	freeLib();
}

bool CComIpc::loadLib()
{
	hLib_ = LoadLibrary(TEXT("YdsComIpc.dll"));
	if (!hLib_) {
		return false;
	}
	return true;
}

void CComIpc::freeLib()
{
	if (hLib_){
		FreeLibrary(hLib_);
		hLib_ = 0;
	}
}

IIpcServer *CComIpc::createIpcServer()
{
	pCreateIpcServerInstance p=(pCreateIpcServerInstance)GetProcAddress(hLib_, IPC_SERVER_FUNC_NAME);
	if (p) {
		return p();
	}
	return 0;
}

IIpcClient *CComIpc::createIpcClient()
{
	pCreateIpcClientInstance p=(pCreateIpcClientInstance)GetProcAddress(hLib_, IPC_CLIENT_FUNC_NAME);
	if (p) {
		return p();
	}
	return 0;
}
