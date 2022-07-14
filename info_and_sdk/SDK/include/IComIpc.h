#pragma once
/**
* @brief IPCヘッダ
*
*/

/**
* @brief コマンド識別子の長さ
*/
#define CMD_NAME_MAX 20

/**
* @brief ログ出力先
*/
enum IpcLogMode
{
	IpcLogMode_None = 0x00,		// なし
	IpcLogMode_File = 0x01,		// ファイル
	IpcLogMode_Console = 0x02,	// コンソール
	IpcLogMode_Debugger = 0x04,	// OutputDebugString
	IpcLogMode_ALL = 0x07		// すべての出力先
};

/**
* @brief コマンドタイプ
*/
enum IpcCmdType
{
	IpcCmdType_Set = TEXT('S'),		// Set
	IpcCmdType_Get = TEXT('G'),		// Get
	IpcCmdType_Cmd = TEXT('C'),		// Command
	IpcCmdType_Reply = TEXT('R'),	// Reply
	IpcCmdType_Trigger = TEXT('T'),	// Trigger
	IpcCmdType_Quit = TEXT('Q')		// Quit
};

/**
* @brief データ型
*/
enum IpcDataType
{
	IpcDataType_Int = TEXT('I'),	//int
	IpcDataType_Float = TEXT('F'),	//float
	IpcDataType_Double = TEXT('D'),	//double
	IpcDataType_Data = TEXT('A')	//byte array
};

/**
* @brief Variantデータ
*/
typedef union VAR {
	INT    ival;	// INT値
	FLOAT  fval;	// FLOAT値
	DOUBLE dval;	// DOUBLE値
	INT    data[1];	// データ先頭アドレス
	//
	VAR(){}
	VAR(INT i) : ival(i){}
	VAR(FLOAT f) : fval(f){}
	VAR(DOUBLE d) : dval(d){}
	VAR(TCHAR *s){ TCHAR *d = (TCHAR*)data; while ((*d++ = *s++) != 0); }
} VAR, *PVAR;

/**
* @brief IPCコールバッククラス
*/
struct IpcCallbackFunc {
	/**
	* @brief IPCコールバック関数
	* @param[in] id メッセージID
	* @param[in] cmdType コマンドタイプ(IpcCmdType)
	* @param[in] cmdName コマンド識別子
	* @param[in] dataType データ型
	* @param[out] pOK BOOL=正常,FALSE=異常
	* @param[out] pVar 値
	* @param[out] pSize 値のサイズ
	*/
	virtual void operator()(CONST USHORT id, CONST CHAR cmdType, LPCSTR cmdName, CONST CHAR dataType,
		LPBOOL pOK, PVAR pVar, LPINT pSize) = 0;
	/**
	* @brief IPCコールバック関数生成
	* @attention serverでコールバックを並列化する時に派生クラスの生成: return new Derived; を記述する
	*/
	virtual IpcCallbackFunc *create(){ return 0; }
};

/**
* @brief IPCサーバークラス
*/
class IIpcServer
{
public:
	/**
	* @brief ログの設定
	* @param[in] mode enum LogMode のOR値
	*            他のDLLログコンソールと併用は不可
	* @param[in] path LogMode_File指定時のログファイルパス
	* @attention ログファイルは新規作成される
	*/
	virtual void setLog(int mode, LPCTSTR path = 0) = 0;
	/**
    * @brief 終了処理
    */
	virtual void destroy() = 0;
	/**
    * @brief 接続を待機する
	* @param[in] pipe_name 接続名
	* @param[in] nmax 接続最大数
	* @return true:正常,false:異常
    */
	virtual bool open(LPCTSTR pipe_name, int nmax = 1) = 0;
	/**
	* @brief 接続を閉じる
	*/
	virtual void close() = 0;
	/**
	* @brief サーバー固有の動作をするためのコールバック関数登録
	* @param[in] OnCallback コールバック関数
	* @attention 直ぐに復帰するコードにすること
	*/
	virtual void addCallbackIpc(IpcCallbackFunc *OnCallback) = 0;
	/**
	* @brief サーバーからクライアントにトリガを発生させる
	* @param[in] cmdName コマンド識別子
	* @param[in] dataType データ型(IpcDataType)
	* @param[in] var データ
	* @param[in] size データサイズ(IpcDataType_Dataのサイズ,固定長型は未指定可)
	* @attention クライアントはコールバック関数を登録していること
	*/
	virtual bool trigger(LPCTSTR cmdName, CONST TCHAR dataType, VAR &var, INT size) = 0;
	virtual bool trigger(LPCTSTR cmdName, CONST TCHAR dataType, VAR var) = 0;
	virtual bool trigger(LPCTSTR cmdName) = 0;
	/**
	* @brief 接続しているクライアントの数を返す
	* @return 接続しているクライアントの数
	*/
	virtual int clients() = 0;

	/**
	* @brief 共有メモリを作成する
	* @param[in] name 共有メモリ名
	* @param[in] size 共有メモリサイズ(byte)
	* @return 共有メモリ先頭アドレス,0:異常
	*/
	virtual LPVOID createShMem(LPCTSTR name, DWORD size) = 0;
	/**
	* @brief 共有メモリを削除
	* @param[in] pMap 共有メモリ先頭アドレス
	*            0:全ての共有メモリを削除
	* @return true:正常,false:異常
	*/
	virtual void deleteShMem(LPVOID pMap = 0) = 0;
};

/**
* @brief IPCクライアントクラス
*/
class IIpcClient
{
public:
	/**
	* @brief ログの設定
	* @param[in] mode enum LogMode のOR値
	*            他のDLLログコンソールと併用は不可
	* @param[in] path LogMode_File指定時のログファイルパス
	* @attention ログファイルは新規作成される
	*/
	virtual void setLog(int mode, LPCTSTR path = 0) = 0;
	/**
    * @brief 終了処理
    */
	virtual void destroy() = 0;
	/**
    * @brief サーバーに接続する
	* @param[in] pipe_name 接続名
	* @return true:正常,false:異常
    */
	virtual bool open(LPCTSTR pipe_name) = 0;
	/**
	* @brief 接続を閉じる
	*/
	virtual void close() = 0;

	virtual	int getInt(LPCTSTR name, PBOOL ok = 0, BOOL wait = TRUE) = 0;
	virtual	float getFloat(LPCTSTR name, PBOOL ok = 0, BOOL wait = TRUE) = 0;
	virtual	double getDouble(LPCTSTR name, PBOOL ok = 0, BOOL wait = TRUE) = 0;
	virtual	int getData(LPCTSTR name, LPVOID data, INT size, PINT ret_size, PBOOL ok = 0, BOOL wait = TRUE) = 0;
	
	virtual	bool setInt(LPCTSTR name, int value, BOOL wait = TRUE) = 0;
	virtual	bool setFloat(LPCTSTR name, float value, BOOL wait = TRUE) = 0;
	virtual	bool setDouble(LPCTSTR name, double value, BOOL wait = TRUE) = 0;
	virtual	bool setData(LPCTSTR name, LPVOID data, INT size, BOOL wait = TRUE) = 0;
	virtual bool setCmd(LPCTSTR name, BOOL wait = TRUE) = 0;

	/**
	* @brief クライアントで非同期にデータを受け取るためのコールバック関数登録
	* @attention 必要なければ登録しなくてよい
	*            直ぐに復帰するコードにすること
	* @param[in] OnCallback コールバック関数
	*/
	virtual void addCallbackIpc(IpcCallbackFunc *OnCallback) = 0;
	virtual void delCallbackIpc() = 0;

	/**
	* @brief 共有メモリを開く
	* @param[in] name 共有メモリ名
	* @return 共有メモリ先頭アドレス,0:異常
	*/
	virtual LPVOID openShMem(LPCTSTR name) = 0;
	/**
	* @brief 共有メモリを閉じる
	* @param[in] pMap 共有メモリ先頭アドレス
	*            0:全ての共有メモリを閉じる
	* @return true:正常,false:異常
	*/
	virtual void closeShMem(LPVOID pMap = 0) = 0;
};

#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

/**
* @brief IPCサーバークラスインスタンス作成関数宣言
*/
extern "C" EXPORT IIpcServer *CreateIpcServerInstance();
/**
* @brief IPCサーバークラスインスタンス作成関数ポインタ
*/
typedef IIpcServer *(*pCreateIpcServerInstance)();
/**
* @brief IPCサーバークラスインスタンス作成関数名
*/
#define IPC_SERVER_FUNC_NAME "CreateIpcServerInstance"

/**
* @brief IPCクライアントクラスインスタンス作成関数宣言
*/
extern "C" EXPORT IIpcClient *CreateIpcClientInstance();
/**
* @brief IPCクライアントクラスインスタンス作成関数ポインタ
*/
typedef IIpcClient *(*pCreateIpcClientInstance)();
/**
* @brief IPCクライアントクラスインスタンス作成関数名
*/
#define IPC_CLIENT_FUNC_NAME "CreateIpcClientInstance"
