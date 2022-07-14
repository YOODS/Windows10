#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <deque>
#include <vector>
#include <fstream>
#include <string>
#include "Elapse.h"
#include "IYCam3D.h"
#include "IDecPhsft.h"
#include "IDecSgbm.h"
#include "function.h"
#include "ComIpc.h"
#include "Event.h"
#include "EventMulti.h"
#include "WaitTimer.h"
#include "TimerThread.h"
#include "point.h"
#include "rgb.h"
#include "png_func.h"
#include "etc.h"
#include "extern.h"
#include "SelfProc.h"
#include "cLog.h"

#define LOG cLog::instance()

using namespace std;

struct CS {
	CRITICAL_SECTION *cs_;
	CS(CRITICAL_SECTION *cs) :cs_(cs) { EnterCriticalSection(cs); }
	~CS() { LeaveCriticalSection(cs_); }
};

#define INITIAL_EXPOSURE_TIME 16800	//初期露光時間
#define INITIAL_GAIN_VALUE 0		//初期ゲイン
#define INITIAL_PRESET_NO 1			//初期プリセット番号

#define INPUT_EVENT "INPUT_EVENT"
static HANDLE hThreadGets = INVALID_HANDLE_VALUE;
static UINT WINAPI gets_loop(void *arg);

static BOOL open_camera(pCreateIYCam3DInstance create_camera);
static int CALLBACK cb_recvimg(int camno, int index, int width, int height, void *mem);
static int CALLBACK cb_lostdev(IYCam3D *dev);
static void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static int pixels = 130;
static int colorf = 0;
static BYTE *img[] = { 0, 0 };			//カメラ画像バッファ
static BYTE *camera_img[] = { 0, 0 };	//3D復元用画像バッファ
static BYTE *img_small[] = { 0, 0 };
static BYTE *color_texture = 0;
static IYCam3D *camera = 0;
static int calib_no = -1;
static UINT_PTR timer_id=0;
static int sbHeight = 0;	//ステータスバーの高さ
//
#define RECT_EVENT "RECT_EVENT"
static HANDLE hEventRect = INVALID_HANDLE_VALUE;
typedef struct{
	int camno, index;
} RectKey;
typedef std::deque<RectKey> RectQue;
RectQue rectq;
CRITICAL_SECTION cs_window, cs_camera;
static HMODULE loadYcamLibrary(pCreateIYCam3DInstance *pcreate_camera);
static HANDLE hThreadRect = INVALID_HANDLE_VALUE;
//
#define TIMER_INTERVAL 1000 //ms
#define TIMER_ID 1000
//Live
static bool live_end = false;
static BOOL live_flg = FALSE;
static bool live_stat = FALSE;
static HANDLE hThreadLive = INVALID_HANDLE_VALUE;
static UINT WINAPI live_loop(void *arg);
static void stopLive();
static void resetSequence();

static bool save_flg = false;	//撮影画像保存フラグ
static bool rg_flg = false;	// レンジグリッド保存フラグ
static bool stdv_flg = false;	//水平方向輝度標準偏差計算フラグ
static bool v_loop = false;		//Vコマンド中
static bool auto_reconnect = false;	//自動再接続

std::vector<float> stdv[2];

//#define DUMMY

#ifdef DUMMY
#define BYTESPP 1
#define IMGWIDTH 1280
#define IMGHEIGHT 1024
#else
#define BYTESPP (colorf ? 3 : 1)
#define IMGWIDTH (camera->imageSize().cx)
#define IMGHEIGHT (camera->imageSize().cy)
#endif
#define WINSIZE (IMGWIDTH*IMGHEIGHT)
#define IMGSIZE (WINSIZE*BYTESPP)
#define IMGWDSZ (IMGWIDTH*BYTESPP)
#define IMGBUFSZ (WINSIZE*3)		//画像バッファサイズ

////#Window関連
//#define WWIDTH_INIT 640
//#define WHEIGHT_INIT 480
#define WWIDTH_INIT (640<IMGWIDTH ? (IMGWIDTH>>1) : IMGWIDTH)
#define WHEIGHT_INIT (640<IMGWIDTH ? (IMGHEIGHT>>1) : IMGHEIGHT)
int Hpos[] = { 0, 0 };
int Vpos[] = { 0, 0 };
float scales[] = { 1.0f, 1.0f };
//
static HWND hWnd[] = {0, 0};
static ATOM atom = 0;
static HANDLE hThread = INVALID_HANDLE_VALUE;
static BYTE *bmp[] = { 0, 0 };
//
static BOOL create_window(int camno);
static UINT WINAPI window_loop(void *arg);
////#

////# 位相シフト関連
static IDecPhsft *phsft = 0;		//位相シフト計算ライブラリ
////# ステレオ相関関連
static IDecSgbm *sgbm = 0;			//ステレオ相関計算ライブラリ

static imageRGB calib_result[2];		//キャリブ結果画像LR
#define CALIB_DIR "..\\calib\\"
#define CAPT_DIR "..\\capt\\"

char key[256];
static int recv_bit[CAMNUM] = { 0 };

#define REG_KEY "Software\\YOODS\\test_ycam"

static Elapse e;     // 撮影処理時間計測 (cb_recvimg()より引越)
/* For IPC */
struct cb_func : public IpcCallbackFunc{
	void operator()(CONST USHORT id, CONST CHAR cmdType, LPCSTR cmdName, CONST CHAR dataType,
		LPBOOL pOK, PVAR pVar, LPINT pSize);
};
IIpcServer *ipc = 0;
static int init_status = 0;	//初期化: 0=前, 1=OK, 2=NG
static VAR static_VAR;
static BOOL static_BOOL;
static PVAR ipcVar = &static_VAR;
static LPBOOL ipcOK = &static_BOOL;
static Event *eventIpc = 0;
static Event eventCaptDone;
static Event eventCaptOneFrame;
EventMulti *events = 0;
Event *eventSelf = 0;
static SelfProc *selfProc = 0;
static INT ypj_addr = 0;
static bool one_shot = false;	//phsft,sgbmで撮る枚数が違うので true=sgbm
#pragma pack(1)
typedef struct{
	BOOL bLive;		//ライブ中
	INT triggerMode;//トリガモード(HW/SW)
	BOOL bProjCon;	//プロジェクター接続
	INT projPatt;	//プロジェクターパターン
	INT phsftMode;	//位相シフト 0:グレー,1:3位相
} SHMEM,*PSHMEM;
#pragma pack()
PSHMEM shmem = 0;
/* For Ipc*/

/* For commercial projector*/
static IIpcClient *ipc_proj = 0;
static bool proj_conn = false;

#define COMPROJ 	if(!proj_conn){ \
						if (proj_conn = ipc_proj->open("YCAM_PROJ")){ \
							printf("projector connected.\n"); \
						} \
					};

class ProjectorThread : public TimerThread {
	int frames = 13;
	int frame;
	WaitTimer wait;
public:
	ProjectorThread():delay_ms(0){}
	int delay_ms;
protected:
	void pre_start(){ frame = 0; }
	bool run(){
		ipc_proj->setInt("C", frame, FALSE);
		wait.sleep(delay_ms * 1000);
		if (!camera->capture())return false;
		return ++frame < frames;
	}
} com_proj;
/* For commercial projector*/

class CameraConnectThread : public TimerThread {
	UINT n;
protected:
	bool run(){
		if (n++){
			return selfProc->exec();
		}
		return true;
	}
public:
	CameraConnectThread(){
		n = 0;
		selfProc->clear();
		selfProc->add("^\n");
	}
};
static CameraConnectThread *cam_con = 0;

const string empty_string;

#define ARG(i) (i < inp.size() ? inp[i] : empty_string)
#define ARGS(i) (ARG(i).c_str())
#define ARGI(i) (atoi(ARGS(i)))
#define ARGF(i) ((float)atof(ARGS(i)))
#define COMP(cmd, i) (_stricmp(ARGS(i), cmd)==0)
#define EQ(cmd) (COMP(cmd, 0))
#define EMPTY(i) (ARG(i).empty())

#define TEST_YCAM_TITLE "test_ycam.exe"

fn_imgst imgst[2];

int _tmain(int argc, _TCHAR *argv[])
{
	LOG.setMode(LogMode_File | LogMode_Console, "test_ycam.log");
	LOG.putf("%s starts...\n", TEST_YCAM_TITLE);

	SetConsoleTitle(TEST_YCAM_TITLE);
	{
		RECT rw, rc;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rw, 0);
		for (;;){
			HWND hwnd = FindWindow(0, TEST_YCAM_TITLE);
			if (hwnd){
				GetWindowRect(hwnd, &rc);
				SetWindowPos(hwnd, hwnd, 0, rw.bottom - (rc.bottom - rc.top), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				break;
			}
		}
	}
	hEventRect = CreateEvent(NULL, TRUE, FALSE, RECT_EVENT);	//手動リセット,非シグナル
	InitializeCriticalSection(&cs_window);
	InitializeCriticalSection(&cs_camera);


	cb_func cb_server;
	CComIpc mgr;
	if (mgr.loadLib()){
		ipc = mgr.createIpcServer();
		//ipc->setLog(IpcLogMode_Console);
		if (ipc->open("YCAM_IPC", 5)){
			eventIpc = new Event;
			ipc->addCallbackIpc(&cb_server);
			shmem = (PSHMEM)ipc->createShMem("YCAM_MEM", 4096);
			shmem->bLive = FALSE;
			shmem->bProjCon = FALSE;
		}
		//
		ipc_proj = mgr.createIpcClient();
		COMPROJ
	}

	//CAMERA情報取得
	FILE *fp;
	fopen_s(&fp,"camtype.txt", "r");
	if (fp) {
		fscanf_s(fp, "%d%d", &pixels, &colorf);
		fclose(fp);
	}
	LOG.putf("size[%d],color[%d]\n", pixels, colorf);

	//YCAM3Dライブラリをロード
	pCreateIYCam3DInstance create_camera;
	HMODULE hCamera = loadYcamLibrary(&create_camera);
	if (hCamera == NULL) {
		LOG.putf("error: LoadLibrary - YdsCamXXXX\n");
		return 1;
	}
	BOOL ok = open_camera(create_camera);
#ifdef DUMMY
	for (int i = 0; i < CAMNUM; ++i){
		img[i] = new BYTE[IMGSIZE];
		img_small[i] = new BYTE[IMGSIZE];
		bmp[i] = new BYTE[IMGSIZE + IMGHEIGHT * 3];	//行ごとのpaddingを考慮
	}
	readImagePNM_LR("../../calib00.pgm", img);
#endif

	if (ok) hThread = (HANDLE)_beginthreadex(NULL, 0, window_loop, NULL, 0, NULL);

	//位相シフトライブラリ初期化
	HMODULE hPhsft = LoadLibrary("YdsDecPhsft.dll");
	if (hPhsft == NULL) {
		LOG.putf("error: LoadLibrary - YdsDecPhsft\n");
	}
	else{
		pCreateIDecPhsftInstance create_phsft = (pCreateIDecPhsftInstance)GetProcAddress(hPhsft, "CreateIDecPhsftInstance");
		if (create_phsft == NULL) {
			LOG.putf("error: GetProcAddress\n");
		}
		else{
			phsft = create_phsft();
		}
	}
	if (ok && phsft){
		//位相シフトのログ出力先
		phsft->setLog(PhsftLogMode_File | PhsftLogMode_Console, "log_phsft.txt");
		bool ok = false;
		for (;;){
			if (!phsft->init("PHSFT.ini")) break;
			//レクティファイパラメータファイルがあるディレクトリを指定
			if (!phsft->setRectParam(CALIB_DIR)){}// break;
			//HMATパラメータファイルがあるディレクトリを指定
			if (!phsft->setHmat(CALIB_DIR)){}// break;
			const int ptn_num = phsft->ptn_num();	//setRectparam()より後
			for (int i = 0; i < 2; ++i){
				camera_img[i] = new BYTE[WINSIZE*ptn_num];
			}
			//#HDR 2020.1.12 del
			//phsft->setCameraImages(camera_img);
			ok = true;
			camera->setTriggerCount(ptn_num);
			if (shmem) shmem->phsftMode = ptn_num == 13 ? 0 : 1;
			break;
		}
		if (!ok){
			LOG.putf("error: initialize phaseshift library\n");
			phsft->destroy();
			phsft = 0;
			FreeLibrary(hPhsft);
		}
	}
	//相関ライブラリ初期化
	HMODULE hSgbm = LoadLibrary("YdsDecSgbm.dll");
	if (hSgbm == NULL) {
		//fLOG.putf("error: LoadLibrary - YdsDecSgbm\n");
	}
	else{
		pCreateIDecSgbmInstance create_sgbm = (pCreateIDecSgbmInstance)GetProcAddress(hSgbm, "CreateIDecSgbmInstance");
		if (create_sgbm == NULL) {
			LOG.putf("error: GetProcAddress\n");
		}
		else{
			sgbm = create_sgbm();
		}
	}
	if (ok && sgbm){
		//ステレオ相関のログ出力先
		sgbm->setLog(SgbmLogMode_File | SgbmLogMode_Console, "log_sgbm.txt");
		bool ok = false;
		for (;;){
			if (!sgbm->init("SGBM.ini")) break;
			for (int i = 0; i < 2; ++i) {
				if (!camera_img[i]) camera_img[i] = new BYTE[WINSIZE];
			}
			//#HDR 2020.1.12 del
			//sgbm->setCameraImages(camera_img);
			if (!sgbm->setRectParam(CALIB_DIR)){}// break;
			if (!sgbm->setHmat(CALIB_DIR)){}// break;
			ok = true;
			break;
		}
		if (!ok){
			LOG.putf("error: initialize stereosgbm library\n");
			sgbm->destroy();
			sgbm = 0;
			FreeLibrary(hSgbm);
		}
	}
	if (img[0]){
#if 0
		printf("test capture -> ");
		//最初にHWトリガにしておくと以下captureが一発でOKになるようです
		camera->setTriggerMode(TrigMode_HW);
		eventCaptOneFrame.reset();
		for (int i = 0; i < 5; ++i){
			camera->setTriggerMode(TrigMode_SW);
			camera->capture();
			if (eventCaptOneFrame.wait(200)) fprintf(stderr, "timeout! ");
			else {
				printf("OK!");
				break;
			}
		}
		puts("");
#endif
		LOG.putf("<HW trigger>\n");
		camera->setTriggerMode(TrigMode_HW);
		if (shmem){
			shmem->triggerMode = TrigMode_HW;
			if (phsft){
				if (phsft->ptn_num() == 13){
					LOG.putf("PHSFT:Use gray code pattern.\n");
					camera->setProjectorFixedPattern(DlpPattPhsft);
					shmem->projPatt = DlpPattPhsft;
				}
				else{
					LOG.putf("PHSFT:Use 3-phase pattern.\n");
					camera->setProjectorFixedPattern(DlpPattPhsft3);
					shmem->projPatt = DlpPattPhsft3;
				}
			}
		}
	}
	events = new EventMulti(EVENTNUM);
	eventSelf = new Event;
	selfProc = new SelfProc(events);
	hThreadGets = (HANDLE)_beginthreadex(NULL, 0, gets_loop, key, 0, NULL);
	if (ok && phsft) init_status = 1;	//OK
	else init_status = 2;	//ERR
	if (ipc)ipc->trigger(_T("INIT"), IpcDataType_Int, init_status == 1 ? 1 : 0);

	DWORD  dwEventNo;
	for (bool loop = true; loop && printf("command: ") && 0 <= (dwEventNo = events->wait());){
		bool raise_event = true;
		v_loop = false;
		e.reset();
		for (char *c = key; *c; ++c) *c = toupper(*c);
		switch (key[0])
		{
		case 'Q':	//終了
			stopLive();
			loop = false;
			break;
		case 'U':
			stdv_flg = (key[1] == '\n' ? 0 : key[1] - '0') ? true : false;
			if (stdv_flg) for (int i = 0; i < 2;++i) stdv[i].resize(IMGHEIGHT, 0);
			break;
		case 'Z':	//画像保存：Z[01]
			save_flg = (key[1] == '\n' ? 0 : key[1] - '0') ? true : false;
			break;
		case 'K':
			execCommand(module_path("cmd_calib.cmd").c_str(), true);
		case 'J':
			//キャリブデータのリロード
			if (phsft){
				phsft->setRectParam(CALIB_DIR);
				phsft->setHmat(CALIB_DIR);
			}
			if (sgbm){
				sgbm->setRectParam(CALIB_DIR);
				sgbm->setHmat(CALIB_DIR);
			}
			puts("<Calibration Result Reloaded>");
			break;
		case 'A':{	//キャリブレーション画像確認
			const int no = atoi(&key[1]);
			char path[MAX_PATH];
			sprintf_s(path, sizeof(path), "%scalib%02d.pgm", CALIB_DIR, no);
			bool ret;
			if (colorf){
				BYTE *buf[] = { new BYTE[WINSIZE], new BYTE[WINSIZE] };
				ret = readImagePNM_LR(path, buf);
				for (int i = 0; i < 2; ++i){
					gray2rgb(buf[i], img[i], WINSIZE);
					delete[] buf[i];
				}
			}
			else ret = readImagePNM_LR(path, img);
			if (ok && ret){
				//キャリブレーション確認用円輪郭ファイルを読む
				//for (int j = no * 2; j < (no + 1) * 2; ++j){
				//	CircleContour &c = circles[j - no * 2];
				//	c.clear();
				//	sprintf_s(path, sizeof(path), "%scircle%02d.bin", CALIB_DIR, j);
				//	FILE *fp;
				//	errno_t err = fopen_s(&fp, path, "rb");
				//	if (!err){
				//		POINTS p;
				//		while (fread((void*)&p, sizeof(POINTS), 1, fp)){
				//			c.push_back(p);
				//		}
				//		fclose(fp);
				//	}
				//	else printf("cannot open: %s\n", path);
				//}
				sprintf_s(path, sizeof(path), "%scalib%02d.png", CALIB_DIR, no);
				imageRGB tmp = read_PNG(path);
				if (!tmp.data){
					LOG.putf("error: no file [%s]\n", path);
					break;
				}
				for (int i = 0; i < 2; ++i){
					calib_result[i].free();
					calib_result[i] = imageRGB(tmp.width >> 1, tmp.height, tmp.colorf());
					if (!tmp.colorf()) break;
				}
				const int whalf = (tmp.width >> 1) * 3;
				BYTE *s = tmp.data;
				BYTE *d[] = { (BYTE*)calib_result[0].data, (BYTE*)calib_result[1].data };
				for (int y = 0; y < tmp.height; ++y){
					memcpy(d[0], s, whalf);
					memcpy(d[1], s + whalf, whalf);
					s += (tmp.width * 3);
					d[0] += whalf;
					d[1] += whalf;
				}
				tmp.free();
				for (int i = 0; i < CAMNUM; ++i){
					InvalidateRect(hWnd[i], NULL, TRUE);
				}
			}
			else {
				*ipcOK = FALSE;
				LOG.putf("cannot open: %s\n", path);
			}
			}break;
			///////////////
		case 'R':{
			rg_flg = (!rg_flg);
			ipcVar->ival = rg_flg ? 1 : 0;
			LOG.putf("<range grid:%s>\n", rg_flg ? "ON" : "OFF");
			// ※常に ipcOK = TRUE、戻り値をGUIのチェックボックスに反映。
			}	break;
		case 'E':{	//露光時間:Ennnn
			if (key[1] == '\n'){
				LOG.putf("exposure time=%d,%d\n", ipcVar->ival = camera->exposureTime(0), camera->exposureTime(1));
			}
			else{
				const int e = atoi(&key[1]);
				LOG.putf("<exposure time:%d>\n", e);
				CS cs(&cs_camera);
				*ipcOK = camera->setExposureTime(0, e);
				camera->setExposureTime(1, e);
				setRegValue(HKEY_CURRENT_USER, REG_KEY, "ExposureTime", e);
			}
			} break;
		case 'G':{	//ゲイン：Gnnnn
			bool ex = (key[1] == 'X');	//'X'=拡張ゲイン
			char *k = ex ? &key[1] : &key[0];
			if (k[1] == '\n'){
				if (ex) LOG.putf("gainx=%d,%d\n", ipcVar->ival = camera->gainx(0), camera->gainx(1));
				else LOG.putf("gain=%d,%d\n", ipcVar->ival = camera->gain(0), camera->gain(1));
			}
			else{
				const int g = atoi(&k[1]);
				LOG.putf("<gain%s:%d>\n", ex ? "x" : "", g);
				CS cs(&cs_camera);
				if (ex){
					*ipcOK = camera->setGainx(0, g);
					camera->setGainx(1, g);
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "Gainx", g);
				}
				else{
					*ipcOK = camera->setGain(0, g);
					camera->setGain(1, g);
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "Gain", g);
				}
			}
			} break;
		case 'C':	//撮影
			calib_no = (key[1] == '\n' ? -1:atoi(&key[1]));
			if (0 <= calib_no) LOG.putf("<capture:%d>\n", calib_no);
			else LOG.putf("<capture>\n");
			stopLive();
			if (camera->triggerMode() == TrigMode_HW){		//HWのときは
				*ipcOK = camera->trigger(DlpModeCapture);	//ストロボ発光して1枚撮る
			}
			else{
				*ipcOK = camera->capture();
			}
			if (0<=calib_no) raise_event = false;	//キャリブの場合はファイル作成後にイベントを上げる
			break;
		case 'S':	//ソフトウェアトリガモード
			puts("<SW trigger>");
			*ipcOK = camera->setTriggerMode(TrigMode_SW);
			if (shmem) shmem->triggerMode = TrigMode_SW;
			break;
		case 'H':	//ハードウェアトリガモード
			puts("<HW trigger>");
			stopLive();
			*ipcOK = camera->setTriggerMode(TrigMode_HW);
			if (shmem) shmem->triggerMode = TrigMode_HW;
			break;
		case 'T':{	//ストロボ発光T[012]
			const int option = key[1] - '0';
			if (option < 0 || 2 < option){
				LOG.putf("invalid option:%d\n", option);
				break;
			}
			if (phsft && option == 2){	//#HDR T20でリセットしない
				if (key[2] != '0') {
					LOG.putf("<reset phaseshift images>\n");
					phsft->reset();
				}
			}
			camera->resetSequence();
			one_shot = option < 2;
			LOG.putf("<trigger:%d>\n", option);
			camera->trigger((DlpMode)option);	//*成功してもfalseのときがあるのでipcOKの設定はしない
			raise_event = false;	//規定枚数撮影後に上げる
			}break;
		case 'P':{	//シリアルポート：P[0-9], IPアドレス
			if (key[1] == '\n'){
				LOG.putf("<YPJPort:close>\n");
				camera->closeComPort();
				if (shmem)shmem->bProjCon = FALSE;
				*ipcOK = TRUE;
			}
			else{
				bool result = false;
				if (strchr(&key[1], '.')){
					ypj_addr = lAddr(&key[1]);
					result = camera->openComPort(ypj_addr);
				}
				else {
					const int port = key[1] - '0';
					LOG.putf("<YPJPort:%d>\n", port);
					result = camera->openComPort(port);
				}
				if (result) {
					char *p = strchr(&key[1], '\n');
					if (p) *p = 0;
					setRegText(HKEY_CURRENT_USER, REG_KEY, "YPJPort", &key[1]);
				}
				else *ipcOK = FALSE;
				if (shmem)shmem->bProjCon = result ? TRUE:FALSE;
			}
			}break;
		case 'L':{	//ライブ開始・終了：L[01]
			const int option = (key[1] == '\n' ? 0: key[1] - '0');
			if (option){
				if (hThreadLive != INVALID_HANDLE_VALUE) break;
				puts("<start live>");
				//camera->setTriggerMode(TrigMode_SW);
				//if (shmem) shmem->triggerMode = TrigMode_SW;
				live_stat = (option == 2);
				if (live_stat){
					imgst[0].clear();
					imgst[1].clear();
				}
				int fr = key[2] == '\n' ? 0 : atoi(&key[2]);
				if (fr) LOG.putf("frame rate: %d\n", fr);
				hThreadLive = (HANDLE)_beginthreadex(NULL, 0, live_loop, (void*)fr, 0, NULL);
			}
			else{
				puts("<stop live>");
				stopLive();
			}
			}break;
		case 'D':{	//プリセット
			if (key[1] == '\n'){
				const int no = camera->presetNo();
				LOG.putf("preset=%d\n", no);
				ipcVar->ival = no;
				*ipcOK = TRUE;
			}
			else{
				const int no = atoi(&key[1]);
				LOG.putf("<preset:%d>\n", no);
				if (*ipcOK = camera->setPresetNo(no)){
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "PresetNo", no);
					puts("OK");
				}
			}
			}break;
		case 'I':{	//I interval[ms] delay[ms]
			COMPROJ
			if (!proj_conn){
				LOG.putf("error: no connection to generic projector\n");
				break;
			}
			int interval = getRegValue(HKEY_CURRENT_USER, REG_KEY, "CP_IntervalTime", 50);
			int delay = getRegValue(HKEY_CURRENT_USER, REG_KEY, "CP_DelayTime", 10);
			char *ctx,*delim = " ";
			char *c = strtok_s(key, delim, &ctx);
			for (int i = 0; c && i < 2;++i){
				if (c = strtok_s(NULL, delim, &ctx)){
					if (i == 0) interval = atoi(c);
					else if (i == 1) delay = atoi(c);
				}
			}
			setRegValue(HKEY_CURRENT_USER, REG_KEY, "CP_IntervalTime", interval);
			setRegValue(HKEY_CURRENT_USER, REG_KEY, "CP_DelayTime", delay);
			LOG.putf("run sequence...interval[%d],delay[%d]\n", interval, delay);
			camera->resetSequence();
			com_proj.delay_ms = delay;
			com_proj.setInterval(interval);
			com_proj.start();
		}break;
		///////////////
		case 'B':{	//プロジェクタ発光強度
			if (key[1] == '\n'){
				LOG.putf("projector brightness=%d\n", ipcVar->ival = camera->projectorBrightness());
			}
			else{
				const int b = atoi(&key[1]);
				LOG.putf("<projector brightness:%d>\n", b);
				if (camera->setProjectorBrightness(b)) {
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "ProjectorBrightness", b);
					puts("OK");
				}
				else {
					*ipcOK = FALSE;
					puts("ERROR");
				}
			}
			}break;
		case 'X':{	//プロジェクター露光時間
			if (key[1] == '\n'){
				LOG.putf("projector exposure time=%d\n", ipcVar->ival = camera->projectorExposureTime());
			}
			else{
				const int e = atoi(&key[1]);
				LOG.putf("<projector exposure time:%d>\n", e);
				if (camera->setProjectorExposureTime(e)) {
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "ProjectorExposureTime", e);
					puts("OK");
				}
				else {
					*ipcOK = FALSE;
					puts("ERR");
				}
			}
			}break;
		case 'W':	//プロジェクター発光間隔
			if (key[1] == '\n'){
				LOG.putf("projector interval=%d\n", ipcVar->ival = camera->projectorFlashInterval());
			}
			else{
				const int i = atoi(&key[1]);
				LOG.putf("<projector interval:%d>\n", i);
				if (camera->setProjectorFlashInterval(i)) {
					setRegValue(HKEY_CURRENT_USER, REG_KEY, "ProjectorFlashInterval", i);
					puts("OK");
				}
				else {
					*ipcOK = FALSE;
					puts("ERR");
				}
			}
			break;
		case 'O':	//プロジェクタをリセット
			puts("<reset projector>");
			*ipcOK = camera->resetProjector();
			break;
		case 'N':{	//プロジェクタパターンN[0-3]
			const int ptn = key[1] - '0';
			LOG.putf("<projector:%d>\n", ptn);
			if (camera->setProjectorFixedPattern((DlpPattern)ptn)){
				if (shmem) shmem->projPatt = ptn;
				puts("OK");
			}
			else {
				*ipcOK = FALSE; puts("ERR");
			}
			}break;
		///////////////
		case 'V':{	//LOOP
			v_loop = true;	//撮影後のイベントを抑制
			int n = atoi(&key[1]);
			if (n <= 0) n = 1;
			if (!phsft) n = 0;
			LOG.putf("start capture loop [%d count]\n", n);
			for (int i = 0; i < n; i++) {
				float *xyz;
				BYTE *rgb;
				bool dorect = true;
				int *rg_id = 0;         // レンジグリッド頂点のピクセルインデックス配列
				LOG.putf("exec %d/%d\n", i + 1, n);
				e.reset();
				camera->resetSequence();
				eventCaptDone.reset();
				phsft->reset();						//#HDR 2021.1.12 add
				one_shot = false;
				camera->trigger((DlpMode)2);
				eventCaptDone.wait();	//撮影が終わるまで待つ
				int num = phsft->exec(&xyz, &rgb, dorect, &rg_id);
				LOG.putf("detected num=%d\n", num);

				char path[MAX_PATH];
				_snprintf_s(path, sizeof(path), _TRUNCATE, "%s%s", CAPT_DIR, "out.ply");
				savePly(path, num, xyz, rgb, rg_flg, IMGWIDTH, IMGHEIGHT, rg_id, color_texture);
			}
			*ipcOK = TRUE;
			}break;

		case 'F':{	//位相シフト計算実行
			if (phsft){
				if (key[1] == '0'){	//#HDR
					LOG.putf("<reset phaseshift images>\n");
					phsft->reset();
					break;
				}
				float *xyz;
				BYTE *rgb;
				bool dorect = true;
				int *rg_id = 0;         // レンジグリッド頂点のピクセルインデックス配列
				int num = phsft->exec(&xyz, &rgb, dorect, &rg_id);
				LOG.putf("detected num=%d\n", num);

				char path[MAX_PATH];
				_snprintf_s(path, sizeof(path), _TRUNCATE, "%s%s", CAPT_DIR, "out.ply");
				savePly(path, num, xyz, rgb, rg_flg, IMGWIDTH, IMGHEIGHT, rg_id, color_texture);
			}
			else {
				*ipcOK = FALSE;
				LOG.putf("error: phaseshift library not initialized.\n");
			}
			}break;
		///////////////
		case 'M':{	//ステレオ相関計算実行
			if (sgbm){
				float *xyz;
				BYTE *rgb;
				bool dorect = true;
				int *rg_id = 0;         // レンジグリッド頂点のピクセルインデックス配列
				sgbm->setCameraImages(camera_img);					//#HDR 2021.1.12 add
				int num = sgbm->exec(&xyz, &rgb, dorect, &rg_id);
				LOG.putf("detected num=%d\n", num);
				if (num){
					char path[MAX_PATH];
					_snprintf_s(path, sizeof(path), _TRUNCATE, "%s%s", CAPT_DIR, "out.ply");
					savePly(path, num, xyz, rgb, rg_flg, IMGWIDTH, IMGHEIGHT, rg_id, color_texture);
				}
			}
			else{
				*ipcOK = FALSE;
				LOG.putf("error: stereosgbm library not initialized.\n");
			}
			}break;
		case '@': {
			for (TSTRLIST inp = split_strdq(&key[1]); !inp.empty();) {
				bool ok;
				if (COMP("SI", 0)) {	//@SI NAME int_value
					if (EMPTY(2)) break;
					LOG.putf("set integer: [%s]=%d -> ", ARGS(1), ARGI(2));
					LOG.putf("%s\n", camera->setIntegerValue(ARGS(1), ARGI(2)) ? "OK" : "ERR");
				}
				else if (COMP("SF", 0)) {	//@SF NAME float_value
					if (EMPTY(2)) break;
					LOG.putf("set float: [%s]=%g -> ", ARGS(1), ARGF(2));
					LOG.putf("%s\n", camera->setFloatValue(ARGS(1), ARGF(2)) ? "OK" : "ERR");
				}
				else if (COMP("SS", 0)) {	//@SS NAME string_value
					if (EMPTY(2)) break;
					LOG.putf("set string: [%s]=\"%s\" -> ", ARGS(1), ARGS(2));
					LOG.putf("%s\n", camera->setStringValue(ARGS(1), ARGS(2)) ? "OK" : "ERR");
				}
				else if (COMP("GI", 0)) {	//@GI NAME
					if (EMPTY(1)) break;
					int v = camera->getIntegerValue(ARGS(1), &ok);
					ipcVar->ival = v;
					LOG.putf("%d : %s\n", v, ok ? "OK":"ERR");
				}
				else if (COMP("GF", 0)) {	//@GF NAME
					if (EMPTY(1)) break;
					float v = camera->getFloatValue(ARGS(1), &ok);
					ipcVar->fval = v;
					LOG.putf("%g : %s\n", v, ok ? "OK" : "ERR");
				}
				else if (COMP("GS", 0)) {	//@GS NAME
					if (EMPTY(1)) break;
					char ret[256];
					ok = camera->getStringValue(ARGS(1), ret, sizeof(ret));
					LOG.putf("\"%s\" : %s\n", ret, ok ? "OK" : "ERR");
				}
				break;
			}
			}break;
		case '.':{
			char *file = &key[1];
			while (*file == ' ') ++file;
			char *p = strchr(file, '\n');
			if (p) *p = '\0';
			ifstream ifs(file);
			if (!ifs){
				LOG.putf("error: open[%s]\n", file);
				break;
			}
			selfProc->clear();
			string row;
			while (getline(ifs, row)){
				size_t pos = row.find_first_not_of(" ");
				if (pos != std::string::npos){
					row = row.substr(pos);	//LTrim
				}
				if (row.front() != '#' && !row.empty()){
					selfProc->add("%s\n", row.c_str());
				}
			}
			if (selfProc->empty()){
				LOG.putf("no commands.\n");
				break;
			}
			selfProc->exec();
			}break;
		case '^':{	//カメラ再接続
			LOG.putf("reconnect camera...\n");
			BOOL ret = open_camera(create_camera);
			if (ret){
				if (phsft){
					if (cam_con && cam_con->isRunning()) cam_con->stop();
					camera->setTriggerCount(phsft->ptn_num());
				}
				camera->setTriggerMode(TrigMode_HW);
				if (ipc) ipc->trigger(_T("LOST"), IpcDataType_Int, FALSE);
			}
			} break;
		case '~':
			if (key[1] == '\n'){}
			else{
				auto_reconnect = (key[1] == 0) ? false : true;
			}
			LOG.putf("<auto reconnect:%s>\n", auto_reconnect ? "ON" : "OFF");
			break;
		case '1':{	//rectifyテスト
#if 0
			for (int i = 0; i < 2; ++i){
				BYTE *camimg = camera_img[i];
				PBYTE s = (PBYTE)img[i];
				memcpy(camimg, s, WINSIZE);
			}
			phsft->setCameraImages(camera_img);
			for (int i = 0; i < 2; ++i){
				BYTE *r = phsft->rbuf(i, 0);
				memcpy(img[i], r, WINSIZE);
				InvalidateRect(hWnd[i], NULL, TRUE);
			}
#endif
			Elapse e;
			for (int i = 0; i < CAMNUM; ++i){
				phsft->rectify(i, -1, img[i]);
				InvalidateRect(hWnd[i], NULL, TRUE);
			}
			LOG.putf("rectify: %.2f msec\n", e.query());
		} break;
		default:
			*ipcOK = FALSE;
			break;
		}
		ZeroMemory(key, sizeof(key));
		events->reset(dwEventNo);
		if (dwEventNo == EVENT_IPC  && raise_event){
			eventIpc->wake();
		}
		if (dwEventNo == EVENT_SELF && raise_event){
			eventSelf->wake();
		}
	}
	Sleep(100);
	if (ipc){
		ipc->destroy();
	}
	delete eventIpc;
	if (ipc_proj)ipc_proj->destroy();
	//後始末
	camera->destroy();
	FreeLibrary(hCamera);
	if (phsft){
		phsft->destroy();
		FreeLibrary(hPhsft);
	}
	if (sgbm){
		sgbm->destroy();
		FreeLibrary(hSgbm);
	}

	return 0;
}

BOOL open_camera(pCreateIYCam3DInstance create_camera)
{
	if (camera) {
		camera->destroy();
	}
	camera = create_camera();
	camera->setLog(CamLogMode_Console | CamLogMode_File, "log_camera.txt");
	int ok = camera->open(pixels, colorf);
	LOG.putf("Left:%s,Right:%s\n", ok & 1 ? "OK" : "NG", ok & 2 ? "OK" : "NG");
	char ypjaddr[32];
	DWORD dwsize = sizeof(ypjaddr);
	getRegText(HKEY_CURRENT_USER, REG_KEY, "YPJPort", ypjaddr, &dwsize, "192.168.222.10");
	int ypjok;
	if (strchr(ypjaddr, '.')) {
		ypj_addr = lAddr(ypjaddr);
		ypjok = camera->openComPort(ypj_addr);
	}
	else {
		ypjok = camera->openComPort(ypjaddr[0] - '0');
	}
	if (shmem)shmem->bProjCon = ypjok ? TRUE : FALSE;
	LOG.putf("YPJ Port open %s(%s).\n", ypjok ? "OK" : "Error", ypjaddr);

	if (ok){
		LOG.putf("image size=%d,%d\n", IMGWIDTH, IMGHEIGHT);
		int ini_et = getRegValue(HKEY_CURRENT_USER, REG_KEY, "ExposureTime", INITIAL_EXPOSURE_TIME);
		int ini_gn = getRegValue(HKEY_CURRENT_USER, REG_KEY, "Gain", INITIAL_GAIN_VALUE);
		int ini_gnx = getRegValue(HKEY_CURRENT_USER, REG_KEY, "Gainx", INITIAL_GAIN_VALUE);
		int ini_pno = getRegValue(HKEY_CURRENT_USER, REG_KEY, "PresetNo", INITIAL_PRESET_NO);
		LOG.putf("set: exposure_time=%d,gain=%d,gainx=%d\n", ini_et, ini_gn, ini_gnx);
		for (int i = 0; i < CAMNUM; ++i){
			if (!img[i]) img[i] = new BYTE[IMGBUFSZ + (colorf ? WINSIZE : 0)];
			ZeroMemory(img[i], IMGBUFSZ + (colorf ? WINSIZE : 0));
			if (!img_small[i]) img_small[i] = new BYTE[IMGBUFSZ];
			if (!bmp[i]) bmp[i] = new BYTE[IMGBUFSZ + IMGHEIGHT * 3];	//表示用：行ごとのpaddingを考慮
			camera->addCallbackRecvImage(i, cb_recvimg, img[i]);
			if ((i + 1) & ok){
				if (!camera->setExposureTime(i, ini_et)){
					LOG.putf("set: presetno=%d\n", ini_pno);
					camera->setPresetNo(ini_pno);
				}
				camera->setGain(i, ini_gn);
				camera->setGainx(i, ini_gnx);
			}
		}
		camera->addCallbackLostDevice(cb_lostdev);
		if (colorf){
			color_texture = new BYTE[IMGBUFSZ];
		}
		return TRUE;
	}
	return FALSE;
}

inline BYTE *gray(BYTE *src, BYTE *dst, int size)
{
	if (colorf){
		rgb2gray(src, dst, size);
		return dst;
	}
	return src;
}

static UINT flg = 0;

//画像取得コールバック
int CALLBACK cb_recvimg(int camno, int index, int width, int height, void *mem)
{
	if (camno < 0 || 1 < camno)return 0;
	//circles[camno].clear();
	calib_result[camno].free();
//	printf("%d-%d;%d,%d,%p\n", camno, index, width, height, mem);
	if (stdv_flg && colorf == 0){
		BYTE *p = (PBYTE)mem;
		for (int y = 0; y < IMGHEIGHT; ++y){
			vector<int>hist(256, 0);
			int sum = 0;
			for (int x = 0; x < IMGWIDTH; ++x){
				sum += p[x];
				++hist[p[x]];
			}
			float N = (float)IMGWIDTH;
			float u = sum / N;
			float v = 0.0f;
			for (int j = 0; j<hist.size(); ++j){
				v += (j - u)*(j - u) * hist[j];
			}
			v /= N;
			float s = sqrtf(v);
			stdv[camno][y] = s;
			p += IMGWIDTH;
		}
	}
	InvalidateRect(hWnd[camno], NULL, TRUE);
	//
	if (live_flg){
		if (live_stat){
			imgst[camno]((PBYTE)mem, width, height, width);
			if (imgst[0].count && imgst[1].count){
				auto i = imgst;
				printf("\r#%06d Left:%3d(%3d-%3d) Right:%3d(%3d-%3d)", i[0].count, i[0].cur_avg, i[0].min_avg, i[0].max_avg, i[1].cur_avg, i[1].min_avg, i[1].max_avg);
			}
		}
		return 0;
	}
//	static Elapse e;
	char path[MAX_PATH];
	char path_c[MAX_PATH];
#if 0
	sprintf_s(path, sizeof(path), "%d-%d.pgm",camno,index);
	saveImagePNM(path, (PBYTE)mem, width, height, 1);
#else
	if (calib_no < 0){
		//キャリブ撮影でない場合
		//printf("%s %d\n", __FUNCTION__, __LINE__);
		if ((phsft && (0 <= index && index<phsft->ptn_num())) ||		//位相シフトは0~12
			 (sgbm && index==0) ){									//ステレオ相関の場合は最初の1枚のみ
			if(camno==0 && index){
				LOG.putf("[%02d]interval: %g msec\n", index, e.query());
				e.reset();
			}
			if (camno == 0 && index == 1 && color_texture){
				memcpy(color_texture, mem, IMGSIZE);	//カラーの場合左目の２枚めをテクスチャ画像として使う
			}
			//ひとまずコピー
			BYTE *camimg = camera_img[camno] + WINSIZE*index;
			//if (colorf)	rgb2gray((BYTE*)mem, camimg, IMGSIZE);	//color -> mono
			//else memcpy(camimg, mem, IMGSIZE);	//nop
			PBYTE s = (PBYTE)mem + (colorf ? IMGSIZE : 0);
			memcpy(camimg, s, WINSIZE);
		}
	}
	flg |= (1 << camno);
	if (flg == 3){
		*path = *path_c = 0;
		if (calib_no < 0){
			//普通の撮影、保存フラグが立っていれば保存
			if (save_flg) {
				sprintf_s(path, sizeof(path), "%scapt%02d.pgm", CAPT_DIR, index);
				if (colorf) sprintf_s(path_c, sizeof(path_c), "%scapt%02d.ppm", CAPT_DIR, index);
			}
		}
		else{
			//キャリブ用撮影
			sprintf_s(path, sizeof(path), "%scalib%02d.pgm", CALIB_DIR, calib_no);
		}
		if (*path){
			BYTE *l = img[0];
			BYTE *r = img[1];
			FILE *fp;
			errno_t err = fopen_s(&fp, path, "wb");
			if (!err){
				vector<BYTE> buf(IMGWDSZ);
				fprintf(fp, "P5\n%d %d\n255\n", IMGWIDTH * 2, IMGHEIGHT);
				for (int j = 0; j < IMGHEIGHT; ++j) {
					fwrite(gray(l, &buf[0], IMGWDSZ), 1, IMGWIDTH, fp);
					fwrite(gray(r, &buf[0], IMGWDSZ), 1, IMGWIDTH, fp);
					l += IMGWDSZ;
					r += IMGWDSZ;
				}
				fclose(fp);
			}
			if (*path_c){
				err = fopen_s(&fp, path_c, "wb");
				if (!err){
					l = img[0];
					r = img[1];
					fprintf(fp, "P6\n%d %d\n255\n", IMGWIDTH * 2, IMGHEIGHT);
					for (int j = 0; j < IMGHEIGHT; ++j) {
						fwrite(l, 1, IMGWDSZ, fp);
						fwrite(r, 1, IMGWDSZ, fp);
						l += IMGWDSZ;
						r += IMGWDSZ;
					}
					fclose(fp);
				}
			}
		}
		if (0 <= calib_no){
			if (eventIpc){
				eventIpc->wake();	//キャリブ終了
			}
		}
		calib_no = -1;
		flg = 0;
	}
#endif
	static int phsft_bits = 0;
	static int sgbm_bits = 0;
	if (phsft_bits == 0 && phsft){
		for (int i = 0; i < phsft->ptn_num(); ++i) phsft_bits |= (1 << i);
	}
	if (sgbm_bits == 0){
		for (int i = 0; i < 1; ++i) sgbm_bits |= (1 << i);
	}
	recv_bit[camno] |= (1 << index);
	int bits = one_shot ? sgbm_bits : phsft_bits;
	bool done = false;
	if (recv_bit[0] == bits && recv_bit[0] == recv_bit[1]){
		LOG.putf("capture done(%d)\n", camera->capturedCount(0));
		resetSequence();
		if (!one_shot && phsft){
			phsft->setCameraImages(camera_img);	//#HDR 2021.1.12 add
		}
		eventCaptDone.wake();
		eventSelf->wake();
		done = true;
		if (eventIpc && !v_loop) {
			eventIpc->wake();	//終了
		}
	}
	if (timer_id) KillTimer(hWnd[0], TIMER_ID);
	if (!done) timer_id = SetTimer(hWnd[0], TIMER_ID, TIMER_INTERVAL, timerProc);
	eventCaptOneFrame.wake();
	return 0;
}

//画像取得コールバック
int CALLBACK cb_lostdev(IYCam3D *dev)
{
	LOG.putf("### device lost ### -> status[%d]\n", dev->isOpened());
	if (ipc) ipc->trigger(_T("LOST"), IpcDataType_Int, TRUE);
	stopLive();
	if (auto_reconnect){
		if (!cam_con) cam_con = new CameraConnectThread;
		cam_con->setInterval(5000);
		cam_con->start();
	}
	return 0;
}

//#define CIRCLES CircleContour &c = (hwnd == hWnd[0] ? circles[0] : circles[1]); \
//				for (auto i = c.begin(); i != c.end(); ++i){ \
//					const int x = (int)(i->x*scale + 0.5f), y = (int)(i->y*scale + 0.5f); \
//					SetPixel(memdc, x, y, RGB(255, 0, 0)); \
//								}
#define STDV	if (stdv_flg){ \
					int max_y=0; float max_s=0.0f;\
					const auto &a = (hwnd == hWnd[0] ? stdv[0] : stdv[1]); \
					int j = 0; \
					for (auto i = a.begin(); i != a.end(); ++i, ++j){ \
						if (max_s < *i){max_s=*i;max_y=j;} \
						const int x = (int)((*i) * scale + 0.5f); \
						const int y = (int)(j * scale + 0.5f); \
						SetPixel(memdc, x, y, RGB(0, 255, 0)); \
					} \
					COLORREF rgb = RGB(255, 0, 0); \
					HPEN hPen = CreatePen(PS_SOLID, 1, rgb); \
					HPEN hOldPen = (HPEN)SelectObject(memdc, hPen); \
					const POINT p1 = { 0, (int)(max_y * scale) }; \
					const POINT p2 = { (int)((IMGWIDTH - 1) * scale), (int)(max_y * scale) }; \
					MoveToEx(memdc, p1.x, p1.y, NULL); \
					LineTo(memdc, p2.x, p2.y); \
					SelectObject(memdc, hOldPen); \
					DeleteObject(hPen); \
				}


//ウインドウプロシジャ
LRESULT WINAPI ConsoleDispWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CS cs(&cs_window);
	int *hpos = (hwnd == hWnd[0] ? &Hpos[0] : &Hpos[1]);
	int *vpos = (hwnd == hWnd[0] ? &Vpos[0] : &Vpos[1]);
	float *scalep = (hwnd == hWnd[0] ? &scales[0] : &scales[1]);
	switch (msg)
	{
	case WM_CREATE:
		ShowWindow(hwnd, SW_SHOW);
		break;
	case WM_SETCURSOR:{
		DWORD ht = LOWORD(lParam);
		LPCSTR cur = NULL;
		switch (ht){
		case  HTCLIENT:
		case HTCAPTION:
			cur = IDC_ARROW; break;
		case HTLEFT:
		case HTRIGHT:
			cur = IDC_SIZEWE; break;
		case HTTOP:
		case HTBOTTOM:
			cur = IDC_SIZENS; break;
		case HTTOPLEFT:
		case HTBOTTOMRIGHT:
			cur = IDC_SIZENWSE; break;
		case HTTOPRIGHT:
		case HTBOTTOMLEFT:
			cur = IDC_SIZENESW; break;
		}
		if (cur) {
			SetCursor(LoadCursor(NULL, cur));
			return TRUE;
		}
		}break;
	case WM_PAINT:{
		RECT r;
		SIZE s;
		GetClientRect(hwnd, &r);
		s.cx = r.right - r.left;
		s.cy = r.bottom - r.top;

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		//if(!live_flg)FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BACKGROUND + 1));
		{	//画像以外の背景(右)を塗りつぶす
			int w = (int)(IMGWIDTH**scalep + 0.5f);
			if (w < s.cx) {
				RECT r = ps.rcPaint;
				r.left = w;
				FillRect(hdc, &r, (HBRUSH)(COLOR_BACKGROUND + 1));
			}
			//画像以外の背景(下)を塗りつぶす
			int h = (int)(IMGHEIGHT**scalep + 0.5f);
			if (h < s.cy) {
				RECT r = ps.rcPaint;
				r.top = h;
				FillRect(hdc, &r, (HBRUSH)(COLOR_BACKGROUND + 1));
			}
		}

		EndPaint(hwnd, &ps);

		int bytespp = 1;
		BYTE *image = 0;
		BYTE *bitmap = 0;
		imageRGB *calib_ret = 0;
		if (hwnd == hWnd[0]){
			calib_ret = &calib_result[0];
			image = calib_ret->data ? calib_ret->data : img[0];
			bitmap = bmp[0];
		}
		else{
			calib_ret = &calib_result[1];
			image = calib_ret->data ? calib_ret->data : img[1];
			bitmap = bmp[1];
		}
		if (colorf || (calib_ret->data && calib_ret->colorf())) bytespp = 3;
		if (!image){
			break;
		}
		HDC hdcImg = GetDC(hwnd);
		HDC memdc = CreateCompatibleDC(hdcImg);
		HBITMAP hbmp = CreateCompatibleBitmap(hdcImg, s.cx, s.cy);
		SelectObject(memdc, hbmp);
		BYTE *image_small = (hwnd == hWnd[0] ? img_small[0] : img_small[1]);
#if 1
		float scale = 1.0f;
		int dx=0, dy = 0;
		if (GetWindowLong(hwnd, GWL_STYLE) & (WS_VSCROLL | WS_HSCROLL)){
			dispImageDC(memdc, image, IMGWIDTH, IMGHEIGHT, bitmap, bytespp, true);
			//円輪郭を描画
			//CIRCLES
			//標準偏差グラフ
			STDV;
			dx = *hpos;	//memdcにしたのでこの2行は不要
			dy = *vpos;
			BitBlt(hdcImg, *hpos, *vpos, IMGWIDTH, IMGHEIGHT, memdc, 0, 0, SRCCOPY);
		}
		else{
			float fw, fh;
			fw = (float)s.cx / (float)IMGWIDTH;
			fh = (float)s.cy / (float)IMGHEIGHT;
			float f = min(fw, fh);
			if (1.0f<f)f = 1.0f;
			scale = f;
			const int w = (int)(IMGWIDTH*f + 0.5f);
			const int h = (int)(IMGHEIGHT*f + 0.5f);
			resizeImage(image, IMGWIDTH, IMGHEIGHT, w, h, image_small, bytespp);
			dispImageDC(memdc, image_small, w, h, bitmap, bytespp, true);
			//円輪郭を描画
			//CIRCLES
			//標準偏差グラフ
			STDV;
			BitBlt(hdcImg, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left + 1, ps.rcPaint.bottom - ps.rcPaint.top + 1, memdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
		}
		*scalep=scale;
#else
		resizeImageBL(image, IMGWIDTH, IMGHEIGHT, s.cx, s.cy, image_small);
		DispImageDC(memdc, image_small, s.cx, s.cy, bitmap);
		BitBlt(hdcImg, 0, 0, s.cx, s.cy, memdc, 0, 0, SRCCOPY);
#endif
		ReleaseDC(hwnd, hdcImg);
		DeleteDC(memdc);
		DeleteObject(hbmp);
		}break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDBLCLK:{
		//break;
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		const LONG scbit = WS_VSCROLL | WS_HSCROLL;
		bool add = false;
		if (style & scbit) style &= ~scbit;
		else{
			add = true;
			style |= scbit;
		}
		SetWindowLong(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		if (add){
			//全体ｰ>原寸のときはクリックした点を中心に表示する
			RECT r; SIZE s;
			GetClientRect(hwnd, &r);
			s.cx = r.right - r.left;
			s.cy = r.bottom - r.top;
			float scl;
			{
				float fw = (float)s.cx / (float)IMGWIDTH;
				float fh = (float)s.cy / (float)IMGHEIGHT;
				float f = min(fw, fh);
				if (1.0f < f)f = 1.0f;
				scl = f;
			}
			int x = (int)(LOWORD(lParam) / scl + 0.5f);
			int y = (int)(HIWORD(lParam) / scl + 0.5f);
			SCROLLINFO si = { 0 };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
			si.nMin = 0;
			//
			si.nMax = IMGWIDTH - s.cx;
			si.nPage = 1;
			if (x < (s.cx >> 1))si.nPos = 0;
			else if (IMGWIDTH - (s.cx >> 1) < x) si.nPos = si.nMax;
			else si.nPos = x - (s.cx >> 1);
			*hpos = -si.nPos;
			SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
			//
			si.nMax = IMGHEIGHT - s.cy + sbHeight;
			si.nPage = 1;
			if (y < (s.cy >> 1))si.nPos = 0;
			else if (IMGHEIGHT - (s.cy >> 1) < y) si.nPos = si.nMax;
			else si.nPos = y - (s.cy >> 1);
			*vpos = -si.nPos;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			{
				POINT p = { s.cx >> 1, s.cy >> 1 };
				ClientToScreen(hwnd, &p);
				SetCursorPos(p.x, p.y);
			}
		}
		}break;
	case WM_SIZE:
		if (GetWindowLong(hwnd, GWL_STYLE) & (WS_VSCROLL | WS_HSCROLL)){
			onSize(hwnd, IMGWIDTH, IMGHEIGHT, hpos, vpos);
		}
		break;
	case WM_VSCROLL:
		if (GetWindowLong(hwnd, GWL_STYLE) & WS_VSCROLL){
			onVScroll(hwnd, LOWORD(wParam),vpos);
		}
		break;
	case WM_HSCROLL:
		if (GetWindowLong(hwnd, GWL_STYLE) & WS_HSCROLL){
			onHScroll(hwnd, LOWORD(wParam), hpos);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

#define CONSOLEDISPCLASS         "ConsoleDispClass"

//ウィンドウ作成
BOOL create_window(int camno)
{
	char title[32];
	sprintf_s(title, sizeof(title), "camera-%d", camno + 1);
	WNDCLASSEX wcx;
	HMODULE hInstance = GetModuleHandle(NULL);
	if (GetClassInfoEx(hInstance, CONSOLEDISPCLASS, &wcx) == 0)
	{
		// Fill in the window class structure with parameters that describe the main window. 
		wcx.cbSize = sizeof(wcx);          // size of structure 
		wcx.style = CS_HREDRAW | CS_NOCLOSE | CS_SAVEBITS | CS_VREDRAW | CS_DBLCLKS | WS_OVERLAPPED;
		wcx.lpfnWndProc = ConsoleDispWndProc;   // points to window procedure 
		wcx.cbClsExtra = 0;                    // no extra class memory 
		wcx.cbWndExtra = 0;                    // no extra window memory 
		wcx.hInstance = hInstance;            // handle to instance 
		wcx.hIcon = NULL;                 // no icon
		wcx.hCursor = NULL;                 // no cursor
		wcx.lpszMenuName = NULL;                 // name of menu resource 
		wcx.lpszClassName = CONSOLEDISPCLASS;     // name of window class 
		wcx.hIconSm = NULL;                 // small class icon 
		wcx.hbrBackground = NULL;

		// Register the window class. 
		atom = RegisterClassEx(&wcx);
	}
	if (atom != NULL)
	{
		int dx = 0;
		if (camno){
			RECT r;
			GetWindowRect(hWnd[0], &r);
			dx = r.right - r.left + 1;
		}
		// create our display window
		hWnd[camno] = CreateWindow(CONSOLEDISPCLASS,        // name of window class 
			title,  // title-bar string 
			WS_OVERLAPPEDWINDOW,      // top-level window 
			dx,//CW_USEDEFAULT,            // default horizontal position 
			0,//CW_USEDEFAULT,            // default vertical position 
			WWIDTH_INIT,             // default width 
			WHEIGHT_INIT,             // default height 
			(HWND)NULL,              // no owner window 
			(HMENU)NULL,             // use class menu 
			hInstance,                // handle to application instance 
			(LPVOID)NULL);            // no window-creation data 
		if (!hWnd[camno])
		{
			UnregisterClass(CONSOLEDISPCLASS, hInstance);
			atom = NULL;
		}
	}

	return (atom != NULL);
}

//Windowsループ
UINT WINAPI window_loop(void *)
{
	for (int i = 0; i < CAMNUM; ++i){
		create_window(i);
		setClientSize(hWnd[i], WWIDTH_INIT, WHEIGHT_INIT);
	}
	//printf("hwnd %d %d\n", (int)hWnd[0], (int)hWnd[1]);

	MSG msg;
	while (0 != GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (UINT)msg.wParam;
}


//ライブループ
UINT WINAPI live_loop(void *arg)
{
	int fr = (int)arg;
	int mode = shmem->triggerMode;
	live_flg = TRUE;
	if (shmem)shmem->bLive = TRUE;
	DWORD ms = mode == TrigMode_SW ? 50 : 500;	//適当なwait
	if (0 < fr) ms = 1000 / fr;
	live_end = false;
	printf("\n");
	Elapse e;
	do{
		{
			CS cs(&cs_camera);
			if (mode == TrigMode_SW) camera->capture();
			else if (mode == TrigMode_HW) camera->trigger(DlpModeCapture);
			else printf("invalid live mode\n");
		}
		Sleep(ms);
		printf("\r%.1ffps  ", 1000.0/e.query());
		e.reset();
	} while (!live_end);
	printf("\n");
	Sleep(ms);
	live_flg = FALSE;
	if (shmem)shmem->bLive = FALSE;
	return 0;
}

//ライブ終了
void stopLive()
{
	if (hThreadLive != INVALID_HANDLE_VALUE) {
		live_end = true;
		DWORD ret = WaitForSingleObject(hThreadLive, 2000);
		if (ret == WAIT_TIMEOUT){
			LOG.putf("time out\n");
			if (!TerminateThread(hThreadLive, 0)){
				LOG.putf("faild to terminte live thread\n");
			}
		}
		live_flg = FALSE;
		if (shmem)shmem->bLive = FALSE;
		CloseHandle(hThreadLive);
		hThreadLive = INVALID_HANDLE_VALUE;
	}
	camera->resetSequence();
}

//カメラインデックスをリセットする
//一連の撮影が終わったら行う
//このアプリではタイマーで行う
void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	KillTimer(hWnd[0], TIMER_ID);
	resetSequence();
}

//YCAM3Dライブラリをロード
//複数あっても最初に見つかったのを返す
HMODULE loadYcamLibrary(pCreateIYCam3DInstance *pcreate_camera)
{
	char search[_MAX_PATH];
	char full[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	GetModuleFileName(NULL, full, _MAX_PATH);
	_splitpath_s(full, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	_makepath_s(search, _MAX_PATH, drive, dir, NULL, NULL);
	strcat_s(search, "*.dll");

	HANDLE hFind;
	WIN32_FIND_DATA fd;
	hFind = FindFirstFile(search, &fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return 0;
	}
	HMODULE hCamera = 0;
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		hCamera = LoadLibrary(fd.cFileName);
		if (hCamera) {
			//試しにインスタンス作成関数ポインタを取得
			pCreateIYCam3DInstance func= (pCreateIYCam3DInstance)GetProcAddress(hCamera, YCAM3DFUNCNAME);
			if (func) {
				LOG.putf("camera library found: [%s]\n", fd.cFileName);
				*pcreate_camera = func;
				break;
			}
			FreeLibrary(hCamera);
			hCamera = 0;
		}
		
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
	return hCamera;
}



LRESULT CALLBACK PickProc(HWND hWindow, UINT msg, WPARAM wp, LPARAM lp);

UINT WINAPI gets_loop(void *arg)
{
	//HANDLE h = OpenEvent(EVENT_ALL_ACCESS, FALSE, INPUT_EVENT);
	for (; fgets((char*)arg, 64, stdin); ){
		ipcVar = &static_VAR;
		ipcOK = &static_BOOL;
		//SetEvent(h);
		events->wake(EVENT_KEY);
	}
	//CloseHandle(h);
	return 0;
}

void cb_func::operator()(CONST USHORT id, CONST TCHAR cmdType, LPCTSTR cmdName, CONST TCHAR dataType, LPBOOL pOK, PVAR pVar, LPINT pSize)
{
	ipcVar = pVar;
	ipcOK = pOK;

	*pOK = TRUE;
	//LOG.putf("### server cb function called ###\n");
	//LOG.putf(" id=%u\n", id);
	//LOG.putf(" cmdType=%c\n", cmdType);
	//LOG.putf(" cmdName=%s\n", cmdName);
	//LOG.putf(" dataType=%c\n", dataType);
	//LOG.putf(" ok=%d\n", *pOK);
	PVAR v = pVar;
	int len = *pSize;
	*key = '\0';
	switch (cmdType){
	case IpcCmdType_Set:	// Set
		if (!strcmp(cmdName, "C")){
			if (0 <= v->ival) _snprintf_s(key, sizeof(key), _TRUNCATE, "C%d\n", v->ival);
			else _snprintf_s(key, sizeof(key), _TRUNCATE, "C\n");
		}
		else if (!strcmp(cmdName, _T("TRIGMODE"))){
			_snprintf_s(key, sizeof(key), _TRUNCATE, "%s\n", v->ival==TrigMode_HW ? "H":"S");
		}
		else if (!strcmp(cmdName, _T("P"))){
			if (v->ival){
				int a = v->ival;
				_snprintf_s(key, sizeof(key), _TRUNCATE, "%s%d.%d.%d.%d\n", "P", a & 0xff, (a >> 8) & 0xff, (a >> 16) & 0xff, (a >> 24) & 0xff);
			}
			else _snprintf_s(key, sizeof(key), _TRUNCATE, "%s\n", "P");
		}
		else {
			_snprintf_s(key, sizeof(key), _TRUNCATE, "%s%d\n", cmdName, v->ival);
		}
		break;
	case IpcCmdType_Get:	// Get
		if (!strcmp(cmdName, "STATUS")){
			v->ival = init_status;
		}
		else if (!strcmp(cmdName, _T("TRIGMODE"))){
			v->ival = camera->triggerMode();
		}
		else if (!strcmp(cmdName, _T("YPJADDR"))){
			v->ival = ypj_addr;
		}
		else{
			_snprintf_s(key, sizeof(key), _TRUNCATE, "%s\n", cmdName);
		}
		break;
	case IpcCmdType_Cmd:	// Command
		_snprintf_s(key, sizeof(key), _TRUNCATE, "%s\n", cmdName);
		break;
	default:
		*pOK = FALSE;
		LOG.putf("$ invalid command type=%d\n", cmdType);
		break;
	}
	if (*key){
		LOG.putf(key);
		//SetEvent(hEventInputIpc);	//IPCキー入力
		events->wake(EVENT_IPC);
		eventIpc->wait();	//終わるまで待つ
		eventIpc->reset();
		//while (WAIT_TIMEOUT != WaitForSingleObject(hEventInputIpc, 1)){ ; }	//念の為、コマンドラインループが待ち状態になるのを確認
		while (!events->waiting(1)){ ; }	//念の為、コマンドラインループが待ち状態になるのを確認
	}
}

void resetSequence()
{
	memset(recv_bit, 0, sizeof(recv_bit));
	camera->resetSequence();
#if 0
	for (int i = 0; i < 2; ++i){
		char path[32];
		sprintf_s(path, sizeof(path), "camera-%d.pgm", i);
		saveImagePNM(path, camera_img[i], IMGWIDTH, IMGHEIGHT * 13, 1);
	}
#endif
	com_proj.stop();
//	printf("sequence was reset\n");
}

