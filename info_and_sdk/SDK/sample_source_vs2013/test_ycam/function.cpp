#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "function.h"

BOOL setClientSize(HWND hWnd, int width, int height)
{
	RECT rw, rc;
	GetWindowRect(hWnd, &rw);
	GetClientRect(hWnd, &rc);

	int new_width = (rw.right - rw.left) - (rc.right - rc.left) + width;
	int new_height = (rw.bottom - rw.top) - (rc.bottom - rc.top) + height;

	return SetWindowPos(hWnd, NULL, 0, 0, new_width, new_height, SWP_NOMOVE | SWP_NOZORDER);
}

static void makeBitmapSpec(BITMAPINFOHEADER *bih, RGBQUAD *col, int width, int height, int bytespp)
{
	int		a;

	///// ビットマップスペック作成
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = width;
	bih->biHeight = height;
	bih->biPlanes = 1;
	bih->biBitCount = bytespp * 8;
	bih->biCompression = bih->biSizeImage = 0;
	bih->biXPelsPerMeter = bih->biYPelsPerMeter = 0;
	bih->biClrUsed = bih->biClrImportant = 0;

	///// カラーテーブル作成
	if (bytespp==1){
		for (a = 0; a < 256; a++){
			col[a].rgbRed = col[a].rgbGreen = col[a].rgbBlue = (BYTE)a;
			col[a].rgbReserved = 0;
		}
	}
}

BOOL dispImageDC(HDC hdc, BYTE *img, int width, int height, BYTE *bmp, int bytespp, bool reverse_order)
{
	if (img == 0){
		return FALSE;
	}
	struct				DIBspec{ BITMAPINFOHEADER bih; RGBQUAD col[256]; }bs;
	HBITMAP				hbmp;

	///// ビットマップスペック作成
	makeBitmapSpec(&bs.bih, bs.col, width, height, bytespp);

	const int stride = (width*bytespp + 3) & 0xFFFFFFFC;

	memset(bmp, 0, stride*height);

	BYTE *d = bmp + stride*(height - 1);
	BYTE *s = img;
	if (bytespp == 3 && reverse_order) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				d[x * 3 + 0] = s[x * 3 + 2];
				d[x * 3 + 1] = s[x * 3 + 1];
				d[x * 3 + 2] = s[x * 3 + 0];
			}
			d -= stride;
			s += width * bytespp;
		}
	}
	else {
		for (int y = 0; y < height; ++y) {
			memcpy(d, s, width*bytespp);
			d -= stride;
			s += width * bytespp;
		}
	}
	///// ビットマップ作成
	hbmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)&bs.bih, CBM_INIT, bmp, (LPBITMAPINFO)&bs, DIB_RGB_COLORS);
	if (hbmp == NULL){
		return FALSE;
	}

	///// 画像描画
	SelectObject(hdc, hbmp);
	DeleteObject(hbmp);

	return TRUE;
}


void resizeImage(const BYTE *srcd, int sw, int sh, int dw, int dh, BYTE *destd, int bytespp) {
	float wfactor = (float)sw / (float)dw;
	float hfactor = (float)sh / (float)dh;
	if (wfactor < 1.0f) wfactor = 1.0f;
	if (hfactor < 1.0f) hfactor = 1.0f;

	const int b = bytespp;
	for (int a = 0; a < b; ++a){
	for (int iy = 0; iy<dh; ++iy) for (int ix = 0; ix<dw; ++ix){
		const float xx = wfactor*ix;
		const float yy = hfactor*iy;

		int x0 = (int)xx;	if (x0<0) x0 = 0;
		int x1 = x0 + 1;	if (x1>sw - 1) x1 = x0;
		int y0 = (int)yy;	if (y0<0) y0 = 0;
		int y1 = y0 + 1;	if (y1>sh - 1) y1 = y0;

			const float xc = x0 - xx;
			const float xn = xx - (x0 + 1);
			const float yc = y0 - yy;
			const float yn = yy - (y0 + 1);
			const float p00 = (float)(unsigned char)srcd[sw*b*y0 + x0*b + a];
			const float p01 = (float)(unsigned char)srcd[sw*b*y0 + x1*b + a];
			const float p10 = (float)(unsigned char)srcd[sw*b*y1 + x0*b + a];
			const float p11 = (float)(unsigned char)srcd[sw*b*y1 + x1*b + a];
		float v = xn * yn * p00
			+ xc * yn * p01
			+ xn * yc * p10
			+ xc * yc * p11;
			if (v < 0.0f)v = 0.0f;
			else if (v > 255.0)v = 255.0f;
			destd[dw*b*iy + ix*b+a] = (char)v;
		}
	}
}

bool saveImagePNM(const char *path, BYTE *imgData, int m, int n, int e){
	errno_t err;
	FILE *fp;
	err = fopen_s(&fp, path, "wb");
	if (err){
		return false;
	}
	fprintf(fp, "%s\n%d %d\n255\n", (e == 1 ? "P5" : "P6"), m, n);
	fwrite(imgData, 1, m*n*e, fp);
	fclose(fp);
	return true;
}



bool readImagePNM(LPCTSTR path, BYTE **imgData, int *m, int *n, int *e) {
	errno_t err;
	FILE *fp;
	err = _tfopen_s(&fp, path, _T("rb"));
	if (err) {
		return false;
	}

	const int BUF_SIZE = 256;
	char buf[BUF_SIZE];
	fgets(buf, BUF_SIZE, fp);

	int mm, nn, ee;
	//img format check.
	if (strncmp(buf, "P5", 2) == 0) {
		ee = 1;
	}
	else if (strncmp(buf, "P6", 2) == 0) {
		ee = 3;
	}
	else {
		return false;
	}
	fgets(buf, BUF_SIZE, fp);
	while ('#' == buf[0]) {
		fgets(buf, BUF_SIZE, fp);
	}

	char *ctx;
	mm = atoi(strtok_s(buf, " ", &ctx));
	nn = atoi(strtok_s(NULL, "\n", &ctx));

	//get gradation
	fgets(buf, BUF_SIZE, fp);

	//get img data
	const int size = mm * nn * ee;

	if (*imgData){}
	else{
		*imgData = new BYTE[size];
	}
	fread(*imgData, 1, size, fp);
	fclose(fp);
	if (m) *m = mm;
	if (n) *n = nn;
	if (e) *e = ee;
	return true;
}

bool readImagePNM_LR(LPCTSTR path, BYTE *imgData[], int *m, int *n, int *e) {
	errno_t err;
	FILE *fp;
	err = _tfopen_s(&fp, path, _T("rb"));
	if (err) {
		return false;
	}

	const int BUF_SIZE = 256;
	char buf[BUF_SIZE];
	fgets(buf, BUF_SIZE, fp);

	int mm, nn, ee;
	//img format check.
	if (strncmp(buf, "P5", 2) == 0) {
		ee = 1;
	}
	else if (strncmp(buf, "P6", 2) == 0) {
		ee = 3;
	}
	else {
		return false;
	}
	fgets(buf, BUF_SIZE, fp);
	while ('#' == buf[0]) {
		fgets(buf, BUF_SIZE, fp);
	}

	char *ctx;
	mm = atoi(strtok_s(buf, " ", &ctx));
	mm >>= 1;
	nn = atoi(strtok_s(NULL, "\n", &ctx));

	//get gradation
	fgets(buf, BUF_SIZE, fp);

	//get img data
	const int size = (mm * nn * ee);

	for (int y = 0; y < nn; ++y){
		fread(imgData[0] + mm*y, 1, mm, fp);
		fread(imgData[1] + mm*y, 1, mm, fp);
	}
	//	fread(*imgData, 1, size, fp);
	fclose(fp);
	if (m) *m = mm;
	if (n) *n = nn;
	if (e) *e = ee;
	return true;
}

//バイナリPLYで保存する
bool savePly(char *path, int num, float *xyz, BYTE *rgb, bool rg_flg, int width, int height, int *rg_id, BYTE *color_img) {
	FILE *fp;
	errno_t err = fopen_s(&fp, path, "wb");
	if (err) {
		fprintf(stderr, "error: open %s\n", path);
		return 0;
	}

	int imgsize = width*height;
	rg_flg = rg_flg && (width > -1);
	rg_flg = rg_flg && (height > -1);
	rg_flg = rg_flg && (rg_id);

	fprintf(fp, "ply\n");
	fprintf(fp, "format binary_little_endian 1.0\n");
	if (rg_flg) { fprintf(fp, "obj_info num_cols %d\n", width); }
	if (rg_flg) { fprintf(fp, "obj_info num_rows %d\n", height); }
	fprintf(fp, "element vertex %u\n", num);
	fprintf(fp, "property float x\n");
	fprintf(fp, "property float y\n");
	fprintf(fp, "property float z\n");
	fprintf(fp, "property uchar red\n");
	fprintf(fp, "property uchar green\n");
	fprintf(fp, "property uchar blue\n");
	if (rg_flg) { fprintf(fp, "element range_grid %d\n", (imgsize)); }
	if (rg_flg) { fprintf(fp, "property list uchar int vertex_indices\n"); }
	fprintf(fp, "end_header\n");

	// PointCloud
#pragma pack(push,1)
	struct ply_bin {
		FLOAT xyz[3];
		BYTE rgb[3];
	};
#pragma pack(pop)
	ply_bin *outdata = new ply_bin[num];
	for (int i = 0; i < num; ++i) {
		outdata[i] = { { xyz[0], xyz[1], xyz[2] }, { rgb[0], rgb[1], rgb[2] } };
		xyz += 3;
		rgb += 3;
	}

	//カラーテクスチャを貼り付ける
	if (0 < width &&  0 < height && rg_id && color_img){
		for (int i = 0, p = 0; i < imgsize; i++) {
			if ((p < num) && (i == rg_id[p])) {
				memcpy(outdata[p].rgb, &color_img[i * 3], 3);
				p++;
			}
		}
	}

	fwrite(outdata, sizeof(ply_bin), num, fp);
	delete[] outdata;

	if (rg_flg) {
		unsigned char flgOFF = 0;
		unsigned char flgON = 1;
		// RangeGrid
		for (int i = 0, p = 0; i < imgsize; i++) {
			if ((p < num) && (i == rg_id[p])) {
				fwrite(&flgON, sizeof(unsigned char), 1, fp);
				fwrite(&p, sizeof(int), 1, fp);
				p++;
			}
			else {
				fwrite(&flgOFF, sizeof(unsigned char), 1, fp);
			}
		}
	}
	/*
	for (int i = 0; i < num; ++i) {
		struct ply_bin ply = { xyz[0], xyz[1], xyz[2], rgb[0], rgb[1], rgb[2] };
		if (fwrite(&ply, sizeof(ply), 1, fp) < 1) {
			fprintf(stderr, "error: write %s\n", path);
			return false;
		}
		xyz += 3;
		rgb += 3;
	}
*/
	fclose(fp);
	printf("saved: %s\n", path);

	return true;
}

// 垂直スクロールバーの処理
void onVScroll(HWND hWnd, int mes, int *Vpos)
{
	// 現在のスクロールバーの状況を取得する
	SCROLLINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_VERT, &info);
	int TrackPos = info.nTrackPos;

	int ymax = info.nMax -info.nPage + 1;          // Posの変化可能な最大値
	int ypos = info.nPos;                           // 変化前のPos位置
	int dy = 0;                                     // 変化量
	int pitch = 1;

	switch (mes)
	{
	case SB_TOP:
		dy = -ypos;
		break;
	case SB_LINEUP:                                 // 行単位のスクロール
		dy = -pitch;
		break;
	case SB_LINEDOWN:
		dy = pitch;
		break;
	case SB_PAGEUP:                                 // ページ単位
		dy = -pitch *16;
		break;
	case SB_PAGEDOWN:
		dy = pitch *16;
		break;
	case SB_BOTTOM:
		dy = ymax - ypos;
		break;
	case SB_THUMBTRACK:                             // ドラッグしているとき
		dy = TrackPos - ypos;
		break;
	}

	// 変化させる必要があると思われるとき
	if (dy != 0)
	{
		ypos += dy;
		if (ypos < 0) ypos = 0;
		if (ypos > ymax) ypos = ymax;

		// 実際に変化させる必要があるとき
		if (ypos != info.nPos)
		{
			dy = ypos - info.nPos;                      // 実際の変化量を保存する
			info.nPos = ypos;                           // ここで、構造体を設定する
			SetScrollInfo(hWnd, SB_VERT, &info, TRUE);  // スクロールバーを設定する
			*Vpos = -ypos;                                // 表示用のコピーも変更する

			// 画面をスクロールさせる
			RECT rc;
			GetClientRect(hWnd, &rc);                   // まず、画面を取得する
			ScrollWindowEx(hWnd, 0, -dy, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWnd);                         // これがないと壊れたように見える
		}
	}
}

// 水平スクロールバーの処理
void onHScroll(HWND hWnd, int mes, int *Hpos)
{
	// 現在のスクロールバーの状況を取得する
	SCROLLINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_HORZ, &info);
	int xmax = info.nMax -info.nPage + 1;            // Posの最大値
	int xpos = info.nPos;                           // 変化前のPos位置
	int dx = 0;                                     // 変化量
	int tPos = info.nTrackPos;
	int pitch = 1;

	switch (mes)
	{
	case SB_LINEUP:                                 // サイズは縦方向と同じにした
		dx = -pitch;
		break;
	case SB_LINEDOWN:
		dx = pitch;
		break;
	case SB_PAGEUP:
		dx = -pitch * 16;
		break;
	case SB_PAGEDOWN:
		dx = pitch * 16;
		break;
	case SB_THUMBTRACK:
		dx = tPos - xpos;
		break;
	}

	// 変化させる必要があると思われるとき
	if (dx != 0)
	{
		xpos += dx;                                   // コピーを変化させる
		if (xpos < 0) xpos = 0;
		if (xpos > xmax) xpos = xmax;

		// 実際に変化させる必要があるとき
		if (xpos != info.nPos)
		{
			dx = xpos - info.nPos;                      // 実際の変化量を保存する
			info.nPos = xpos;                           // スクロールバーに設定する
			SetScrollInfo(hWnd, SB_HORZ, &info, TRUE);
			*Hpos = -xpos;                                // 表示用のコピーを変更する

			// 画面をスクロールさせる
			RECT rc;
			GetClientRect(hWnd, &rc);
			ScrollWindowEx(hWnd, -dx, 0, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWnd);
		}
	}
}

// ウィンドウサイズ変更処理
void onSize(HWND hWnd, int w, int h, int *Hpos, int *Vpos)
{
	return;
	RECT r; SIZE s;
	GetClientRect(hWnd, &r);
	s.cx = r.right - r.left + 1;
	s.cy = r.bottom - r.top + 1;

	SCROLLINFO info;

	GetScrollInfo(hWnd, SB_HORZ, &info);
	// 水平スクロールバー
	info.nMax = w - s.cx;
	if ((int)info.nPage > info.nMax + 1){
		info.nPage = info.nMax + 1;
	}
	int xmax = info.nMax - info.nPage + 1;            // Posの最大値
	if (*Hpos > xmax) *Hpos = xmax;
	info.nPos = *Hpos;
	SetScrollInfo(hWnd, SB_HORZ, &info, TRUE);

	// 垂直スクロールバー
	GetScrollInfo(hWnd, SB_VERT, &info);
	info.nMax = h - s.cy;
	if ((int)info.nPage > info.nMax + 1){
		info.nPage = info.nMax + 1;
	}
	int ymax = info.nMax - info.nPage + 1;
	if (*Vpos > ymax) *Vpos = ymax;
	info.nPos = *Vpos;
	SetScrollInfo(hWnd, SB_VERT, &info, TRUE);
}

bool execCommand(const char *cmd, bool wait)
{
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si;// = { sizeof(STARTUPINFO) };
	GetStartupInfo(&si);
	BOOL ret = CreateProcess(NULL, (char*)cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	if (!ret)return false;
	CloseHandle(pi.hThread);
	if (wait){
		WaitForSingleObject(pi.hProcess, INFINITE);
	}
	CloseHandle(pi.hProcess);
	return true;
}


tstring module_dir() {
	TCHAR appdir[_MAX_PATH];
	TCHAR full[_MAX_PATH];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	GetModuleFileName(NULL, full, _MAX_PATH);
	_tsplitpath_s(full, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	_tmakepath_s(appdir, _MAX_PATH, drive, dir, NULL, NULL);
	return tstring(appdir);
}

tstring module_path(LPCTSTR file) {
	tstring path = module_dir();
	path.append(file);
	return path;
}

// レジストリ設定 (DWORD)
// 
// [引数]
// root : HKEY_CLASS_ROOT等 (WinReg.hを参照)
// path : レジストリのパス情報
// key  : レジストリのキー
// dwNewValue : 設定値
// 
// [返値]
// 書き込み成功 : TRUE
// 書き込み失敗 : FALSE
BOOL setRegValue(HKEY root, char *path, char *key, DWORD dwNewValue)
{
	HKEY hRegKey;
	DWORD dwDisp;
	BOOL bRet = FALSE;

	hRegKey = NULL;

	if (RegCreateKeyEx(root, path, NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRegKey, &dwDisp) == ERROR_SUCCESS) {
		if (RegSetValueEx(hRegKey, key, 0, REG_DWORD, (LPBYTE)&dwNewValue, sizeof(DWORD)) == ERROR_SUCCESS) {
			bRet = TRUE;
		}
		RegCloseKey(hRegKey);
	}

	return bRet;
}

// レジストリ取得 (DWORD)
//
// [引数]
// root : HKEY_CLASS_ROOT等 (WinReg.hを参照)
// path : レジストリのパス情報
// key  : レジストリのキー
// dwDefault : 取得できなかった場合のデフォルト値
// 
// [返値]
// 取得できた場合 : 取得値
// 取得できなかった場合 : 引数で指定したデフォルト値
DWORD getRegValue(HKEY root, char *path, char *key, DWORD dwDefault)
{
	HKEY hRegKey;
	DWORD dwRet;
	DWORD dwType = REG_DWORD;
	DWORD dwBufferSize = sizeof(DWORD);

	hRegKey = NULL;
	dwRet = dwDefault; // RegOpenが失敗した場合の準備

	if (RegOpenKeyEx(root, path, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hRegKey, key, 0, &dwType, (LPBYTE)&dwRet, &dwBufferSize) != ERROR_SUCCESS) {
			dwRet = dwDefault; // エラーの場合はデフォルト値
		}
		RegCloseKey(hRegKey);
	}

	return dwRet;
}

// レジストリ設定 (テキスト)
// 
// [引数]
// root : HKEY_CLASS_ROOT等 (WinReg.hを参照)
// path : レジストリのパス情報
// key  : レジストリのキー
// szBuffer : 設定内容
// 
// [返値]
// 書き込み成功 : TRUE
// 書き込み失敗 : FALSE

BOOL setRegText(HKEY root, char *path, char *key, char *szBuffer) {
	HKEY hRegKey;
	DWORD dwDisp;
	DWORD dwLength;
	BOOL bRet = FALSE;

	hRegKey = NULL;
	dwLength = lstrlen(szBuffer);

	if (RegCreateKeyEx(root, path, NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRegKey, &dwDisp) == ERROR_SUCCESS) {
		if (RegSetValueEx(hRegKey, key, 0, REG_SZ, (LPBYTE)szBuffer, dwLength) == ERROR_SUCCESS) {
			bRet = TRUE;
		}
		RegCloseKey(hRegKey);
	}

	return bRet;
}

// レジストリ取得 (テキスト)
//
// [引数]
// root : HKEY_CLASS_ROOT等 (WinReg.hを参照)
// path : レジストリのパス情報
// key  : レジストリのキー
// szBuffer : 取得文字列を保存するためのバッファ
// dwBufferSize : バッファのサイズ
// szDefault : 取得できなかった場合のデフォルト文字列
// 
// [返値]
// 取得できた場合 : TRUE
// 取得できなかった場合 : FALSE

BOOL getRegText(HKEY root, char *path, char *key, char *szBuffer, LPDWORD dwBufferSize, char *szDefault){
	HKEY hRegKey;
	BOOL bRet = FALSE;
	DWORD dwType = REG_SZ;

	hRegKey = NULL;
	lstrcpy(szBuffer, szDefault); // RegOpen関数の失敗時に備える

	if (RegOpenKeyEx(root, path, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hRegKey, key, 0, &dwType, (LPBYTE)szBuffer, dwBufferSize) == ERROR_SUCCESS) {
			bRet = TRUE;
		}
		else {
			lstrcpy(szBuffer, szDefault); // エラーの場合はデフォルト値
		}
		RegCloseKey(hRegKey);
	}

	return bRet;
}

// IPアドレス文字列をintに変換
//
// [引数]
// p : 文字列形式のアドレス
// 
// [返値]
// 変換されたアドレス

int lAddr(char *p) {
	union {
		int port;
		unsigned char chr[4];
	} addr;
	for (int i = 0; i < 4; i++) {
		addr.chr[i] = atoi(p);
		p = strchr(p, '.') + 1;
	}
	return addr.port;
}

BYTE *rgb2gray(BYTE *src, BYTE *dst, int size)
{
	for (int i = 0; i < size; i += 3, ++dst) {
		*dst = ((306 * src[i] + 601 * src[i + 1] + 117 * src[i + 2]) >> 10);	// (>>10) = (/1024)
	}
	return dst;
}

BYTE * gray2rgb(BYTE *src, BYTE *dst, int size)
{
	for (int i = 0; i < size; ++i, dst+=3) {
		dst[0] = src[i];
		dst[1] = src[i];
		dst[2] = src[i];
	}
	return dst;
}

TSTRLIST split_strdq(LPCTSTR str, TCHAR delim) {
	TSTRLIST result;
	std::string tmp;
	bool indq = false;
	for (LPCTSTR p = str; *p && *p != _T('\n') && *p != _T('\r'); ++p) {
		if (*p == _T('\"')) {
			indq = !indq;
		}
		else if (*p == delim) {
			if (indq) {
				tmp.push_back(*p);
			}
			else {
				result.push_back(tmp);
				tmp.clear();
			}
		}
		else {
			tmp.push_back(*p);
		}
	}
	if (!tmp.empty()) {
		result.push_back(tmp);
	}
	return result;
}
