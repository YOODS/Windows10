#pragma once
#include <string>
#include <vector>

typedef std::basic_string<TCHAR> tstring;
typedef std::vector<tstring> TSTRLIST;

//reverse_order = false : BGR, true : RGB
extern BOOL dispImageDC(HDC hdc, BYTE *img, int width, int height, BYTE *bmp, int bytespp = 1, bool reverse_order = false);
extern BOOL setClientSize(HWND hWnd, int width, int height);
extern void resizeImage(const BYTE *srcd, int sw, int sh, int dw, int dh, BYTE *destd,int bytespp=1);
extern void onVScroll(HWND hWnd, int mes, int *Vpos);
extern void onHScroll(HWND hWnd, int mes, int *Hpos);
extern void onSize(HWND hWnd, int w, int h, int *Hpos, int *Vpos);

extern bool saveImagePNM(const char *path, BYTE *imgData, int m, int n, int e);
extern bool readImagePNM(LPCTSTR path, BYTE **imgData, int *m = 0, int *n = 0, int *e = 0);
extern bool readImagePNM_LR(LPCTSTR path, BYTE *imgData[], int *m = 0, int *n = 0, int *e = 0);
extern bool savePly(char *path, int num, float *xyz, BYTE *rgb, bool rg = false, int width = -1, int height = -1, int *rg_id = 0, BYTE *color_img = 0);
extern BYTE *rgb2gray(BYTE *src, BYTE *dst, int size);
extern BYTE *gray2rgb(BYTE *src, BYTE *dst, int size);

extern bool execCommand(const char *cmd, bool wait=true);

extern tstring module_dir();
extern tstring module_path(LPCTSTR file);

BOOL setRegValue(HKEY root, char *path, char *key, DWORD dwNewValue);
DWORD getRegValue(HKEY root, char *path, char *key, DWORD dwDefault);
BOOL setRegText(HKEY root, char *path, char *key, char *szBuffer);
BOOL getRegText(HKEY root, char *path, char *key, char *szBuffer, LPDWORD dwBufferSize, char *szDefault);
int lAddr(char *p);

extern TSTRLIST split_strdq(LPCTSTR str, TCHAR delim = TEXT(' '));
