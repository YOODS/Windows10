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

	///// �r�b�g�}�b�v�X�y�b�N�쐬
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = width;
	bih->biHeight = height;
	bih->biPlanes = 1;
	bih->biBitCount = bytespp * 8;
	bih->biCompression = bih->biSizeImage = 0;
	bih->biXPelsPerMeter = bih->biYPelsPerMeter = 0;
	bih->biClrUsed = bih->biClrImportant = 0;

	///// �J���[�e�[�u���쐬
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

	///// �r�b�g�}�b�v�X�y�b�N�쐬
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
	///// �r�b�g�}�b�v�쐬
	hbmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)&bs.bih, CBM_INIT, bmp, (LPBITMAPINFO)&bs, DIB_RGB_COLORS);
	if (hbmp == NULL){
		return FALSE;
	}

	///// �摜�`��
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

//�o�C�i��PLY�ŕۑ�����
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

	//�J���[�e�N�X�`����\��t����
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

// �����X�N���[���o�[�̏���
void onVScroll(HWND hWnd, int mes, int *Vpos)
{
	// ���݂̃X�N���[���o�[�̏󋵂��擾����
	SCROLLINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_VERT, &info);
	int TrackPos = info.nTrackPos;

	int ymax = info.nMax -info.nPage + 1;          // Pos�̕ω��\�ȍő�l
	int ypos = info.nPos;                           // �ω��O��Pos�ʒu
	int dy = 0;                                     // �ω���
	int pitch = 1;

	switch (mes)
	{
	case SB_TOP:
		dy = -ypos;
		break;
	case SB_LINEUP:                                 // �s�P�ʂ̃X�N���[��
		dy = -pitch;
		break;
	case SB_LINEDOWN:
		dy = pitch;
		break;
	case SB_PAGEUP:                                 // �y�[�W�P��
		dy = -pitch *16;
		break;
	case SB_PAGEDOWN:
		dy = pitch *16;
		break;
	case SB_BOTTOM:
		dy = ymax - ypos;
		break;
	case SB_THUMBTRACK:                             // �h���b�O���Ă���Ƃ�
		dy = TrackPos - ypos;
		break;
	}

	// �ω�������K�v������Ǝv����Ƃ�
	if (dy != 0)
	{
		ypos += dy;
		if (ypos < 0) ypos = 0;
		if (ypos > ymax) ypos = ymax;

		// ���ۂɕω�������K�v������Ƃ�
		if (ypos != info.nPos)
		{
			dy = ypos - info.nPos;                      // ���ۂ̕ω��ʂ�ۑ�����
			info.nPos = ypos;                           // �����ŁA�\���̂�ݒ肷��
			SetScrollInfo(hWnd, SB_VERT, &info, TRUE);  // �X�N���[���o�[��ݒ肷��
			*Vpos = -ypos;                                // �\���p�̃R�s�[���ύX����

			// ��ʂ��X�N���[��������
			RECT rc;
			GetClientRect(hWnd, &rc);                   // �܂��A��ʂ��擾����
			ScrollWindowEx(hWnd, 0, -dy, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWnd);                         // ���ꂪ�Ȃ��Ɖ�ꂽ�悤�Ɍ�����
		}
	}
}

// �����X�N���[���o�[�̏���
void onHScroll(HWND hWnd, int mes, int *Hpos)
{
	// ���݂̃X�N���[���o�[�̏󋵂��擾����
	SCROLLINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_HORZ, &info);
	int xmax = info.nMax -info.nPage + 1;            // Pos�̍ő�l
	int xpos = info.nPos;                           // �ω��O��Pos�ʒu
	int dx = 0;                                     // �ω���
	int tPos = info.nTrackPos;
	int pitch = 1;

	switch (mes)
	{
	case SB_LINEUP:                                 // �T�C�Y�͏c�����Ɠ����ɂ���
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

	// �ω�������K�v������Ǝv����Ƃ�
	if (dx != 0)
	{
		xpos += dx;                                   // �R�s�[��ω�������
		if (xpos < 0) xpos = 0;
		if (xpos > xmax) xpos = xmax;

		// ���ۂɕω�������K�v������Ƃ�
		if (xpos != info.nPos)
		{
			dx = xpos - info.nPos;                      // ���ۂ̕ω��ʂ�ۑ�����
			info.nPos = xpos;                           // �X�N���[���o�[�ɐݒ肷��
			SetScrollInfo(hWnd, SB_HORZ, &info, TRUE);
			*Hpos = -xpos;                                // �\���p�̃R�s�[��ύX����

			// ��ʂ��X�N���[��������
			RECT rc;
			GetClientRect(hWnd, &rc);
			ScrollWindowEx(hWnd, -dx, 0, &rc, &rc, NULL, NULL, SW_ERASE | SW_INVALIDATE);
			UpdateWindow(hWnd);
		}
	}
}

// �E�B���h�E�T�C�Y�ύX����
void onSize(HWND hWnd, int w, int h, int *Hpos, int *Vpos)
{
	return;
	RECT r; SIZE s;
	GetClientRect(hWnd, &r);
	s.cx = r.right - r.left + 1;
	s.cy = r.bottom - r.top + 1;

	SCROLLINFO info;

	GetScrollInfo(hWnd, SB_HORZ, &info);
	// �����X�N���[���o�[
	info.nMax = w - s.cx;
	if ((int)info.nPage > info.nMax + 1){
		info.nPage = info.nMax + 1;
	}
	int xmax = info.nMax - info.nPage + 1;            // Pos�̍ő�l
	if (*Hpos > xmax) *Hpos = xmax;
	info.nPos = *Hpos;
	SetScrollInfo(hWnd, SB_HORZ, &info, TRUE);

	// �����X�N���[���o�[
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

// ���W�X�g���ݒ� (DWORD)
// 
// [����]
// root : HKEY_CLASS_ROOT�� (WinReg.h���Q��)
// path : ���W�X�g���̃p�X���
// key  : ���W�X�g���̃L�[
// dwNewValue : �ݒ�l
// 
// [�Ԓl]
// �������ݐ��� : TRUE
// �������ݎ��s : FALSE
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

// ���W�X�g���擾 (DWORD)
//
// [����]
// root : HKEY_CLASS_ROOT�� (WinReg.h���Q��)
// path : ���W�X�g���̃p�X���
// key  : ���W�X�g���̃L�[
// dwDefault : �擾�ł��Ȃ������ꍇ�̃f�t�H���g�l
// 
// [�Ԓl]
// �擾�ł����ꍇ : �擾�l
// �擾�ł��Ȃ������ꍇ : �����Ŏw�肵���f�t�H���g�l
DWORD getRegValue(HKEY root, char *path, char *key, DWORD dwDefault)
{
	HKEY hRegKey;
	DWORD dwRet;
	DWORD dwType = REG_DWORD;
	DWORD dwBufferSize = sizeof(DWORD);

	hRegKey = NULL;
	dwRet = dwDefault; // RegOpen�����s�����ꍇ�̏���

	if (RegOpenKeyEx(root, path, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hRegKey, key, 0, &dwType, (LPBYTE)&dwRet, &dwBufferSize) != ERROR_SUCCESS) {
			dwRet = dwDefault; // �G���[�̏ꍇ�̓f�t�H���g�l
		}
		RegCloseKey(hRegKey);
	}

	return dwRet;
}

// ���W�X�g���ݒ� (�e�L�X�g)
// 
// [����]
// root : HKEY_CLASS_ROOT�� (WinReg.h���Q��)
// path : ���W�X�g���̃p�X���
// key  : ���W�X�g���̃L�[
// szBuffer : �ݒ���e
// 
// [�Ԓl]
// �������ݐ��� : TRUE
// �������ݎ��s : FALSE

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

// ���W�X�g���擾 (�e�L�X�g)
//
// [����]
// root : HKEY_CLASS_ROOT�� (WinReg.h���Q��)
// path : ���W�X�g���̃p�X���
// key  : ���W�X�g���̃L�[
// szBuffer : �擾�������ۑ����邽�߂̃o�b�t�@
// dwBufferSize : �o�b�t�@�̃T�C�Y
// szDefault : �擾�ł��Ȃ������ꍇ�̃f�t�H���g������
// 
// [�Ԓl]
// �擾�ł����ꍇ : TRUE
// �擾�ł��Ȃ������ꍇ : FALSE

BOOL getRegText(HKEY root, char *path, char *key, char *szBuffer, LPDWORD dwBufferSize, char *szDefault){
	HKEY hRegKey;
	BOOL bRet = FALSE;
	DWORD dwType = REG_SZ;

	hRegKey = NULL;
	lstrcpy(szBuffer, szDefault); // RegOpen�֐��̎��s���ɔ�����

	if (RegOpenKeyEx(root, path, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hRegKey, key, 0, &dwType, (LPBYTE)szBuffer, dwBufferSize) == ERROR_SUCCESS) {
			bRet = TRUE;
		}
		else {
			lstrcpy(szBuffer, szDefault); // �G���[�̏ꍇ�̓f�t�H���g�l
		}
		RegCloseKey(hRegKey);
	}

	return bRet;
}

// IP�A�h���X�������int�ɕϊ�
//
// [����]
// p : ������`���̃A�h���X
// 
// [�Ԓl]
// �ϊ����ꂽ�A�h���X

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
