#pragma once
#include <windows.h>
#include <stdio.h>
#include <iostream>

enum LogMode
{
	LogMode_None = 0x00,	/// なし
	LogMode_File = 0x01,	/// ファイル
	LogMode_Console = 0x02,	/// コンソール
	LogMode_Debugger = 0x04,	/// OutputDebugString
	LogMode_ALL = 0x07
};

class cLog
{
	int mode_;
	LPSTR log_path_;
	BOOL new_console_;
	FILE *con_;
	cLog() : mode_(LogMode_None), log_path_(0), new_console_(FALSE){}
	~cLog(){
		if ((mode_ & LogMode_Console) && new_console_){
			fclose(con_);
			FreeConsole();
		}
		delete[] log_path_;
	}
public:
	static cLog &instance(){
	    static cLog inst;
	    return inst;
	}
	void setMode(int mode, LPCSTR path=0);
	void putf(LPCSTR str, ...);
};

inline void cLog::setMode(int mode, LPCSTR path){
	mode_=mode;
	if(mode_ & LogMode_Console){
		new_console_ = AttachConsole(ATTACH_PARENT_PROCESS);
		if (!new_console_) {
			new_console_=AllocConsole();
		}
		if(new_console_){
			freopen_s(&con_, "CONOUT$", "w", stdout); 
		}
	}
	if((mode_ & LogMode_File) && path){
		log_path_ = new CHAR[MAX_PATH];
		strcpy_s(log_path_, MAX_PATH, path);
		std::locale::global(std::locale("japanese", std::locale::ctype));
		std::ofstream ofs(log_path_, std::ios::trunc);
	}
}

inline void cLog::putf(LPCSTR str, ...) {
	if (mode_ == LogMode_None) {
		return;
	}
	//
	CHAR buf[512];
	va_list args;
	va_start(args, str);
	vsprintf_s(buf, str, args);
	va_end(args);
	//
	if (mode_ & LogMode_Debugger) {
		OutputDebugString(buf);
	}
	if (mode_ & LogMode_File) {
		if (log_path_) {
			SYSTEMTIME st;
			GetLocalTime(&st);
			char tmbuf[64];
			sprintf_s(tmbuf, sizeof(tmbuf),"[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond,st.wMilliseconds);
			std::ofstream ofs(log_path_, std::ios::app);
			if (ofs) {
				ofs << tmbuf << buf;
			}
		}
	}
	if (mode_ & LogMode_Console) {
		std::cout << buf;
	}
}

