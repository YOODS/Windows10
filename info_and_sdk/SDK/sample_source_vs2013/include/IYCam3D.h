#pragma once
/**
* @brief YCAM3Dヘッダ
*
*/

/**
* @brief カメラ数
*/
#define CAMNUM 2

/**
* @brief ログ出力先
*/
enum CamLogMode
{
	CamLogMode_None = 0x00,		/// なし
	CamLogMode_File = 0x01,		/// ファイル
	CamLogMode_Console = 0x02,	/// コンソール
	CamLogMode_Debugger = 0x04,	/// OutputDebugString
	CamLogMode_ALL = 0x07		/// すべての出力先
};

/**
* @brief トリガーモード
*/
enum TrigMode
{
	TrigMode_HW,	/// HWトリガ
	TrigMode_SW		/// SWトリガ
};

/**
* @brief DLP発光モード
*/
enum DlpMode
{
	DlpModeOnce,		///シングル発光(キャプチャなし / X枚目のパタンを照射; X = プロジェクター固定パターン)
	DlpModeCapture,		///シングル発光(キャプチャあり / 1枚目のパタンを照射)
	DlpModeCont			///連続発光(キャプチャあり / 1から規定数のパタンを連続照射)
};

/**
* @brief DLPパターン
*/
enum DlpPattern
{
	DlpPattFixed,	//固定パターン
	DlpPattPhsft,	//位相シフト用
	DlpPattStrobe,	//ストロボ撮影用
	DlpPattFocus,	//ピント合わせ用
	DlpPattStereo,	//相関用
	DlpPattPhsft3	//位相シフト用(3位相版)
};

/**
* @brief 画像データコールバック関数型
* @param[out] camno [0]:左カメラ,[1]:右カメラ
* @param[out] index 画像インデックス(0～)
* @param[out] width 幅
* @param[out] height 高さ
* @param[out] mem データアドレス
*/
typedef int (CALLBACK *RecvImageFunc)(int camno, int index, int width, int height, void *mem);

class IYCam3D;
/**
* @brief カメラ切断コールバック関数型
* @param[out] IYCam3D カメラクラス
*/
typedef int (CALLBACK *DeviceLostFunc)(IYCam3D *dev);

/**
* @brief YCAM3Dクラス
*/
class IYCam3D
{
public:
	/**
	* @brief ログの設定
	* @param[in] mode enum LogMode のOR値
	*            他のDLLログコンソールと併用は不可
	* @param[in] path LogMode_File指定時のログファイルパス
	* @attention ログファイルは新規作成される
	*/
	virtual void setLog(int mode, LPCSTR path = 0) = 0;
	/**
    * @brief 終了処理
    */
	virtual void destroy() = 0;
	/**
    * @brief カメラを開く
	* @return !0:正常,0:異常
	*          1:左カメラOK,2:右カメラOKの論理和
    */
	virtual int open(int,int) = 0;
	/**
	* @brief カメラが開かれているか
	* @return true:済,false:未
	*/
	virtual bool isOpened() = 0;
	/**
	* @brief カメラを閉じる
	* @return true:正常,false:異常
	*/
	virtual void close() = 0;
	/**
	* @brief 画像サイズ
	* @return cx:幅,cy:高さ
	*/
	virtual SIZE imageSize() = 0;
	/**
	* @brief SW/HWトリガによる撮影
	* @return true:正常,false:異常
	* @attention HWトリガの場合は待ち状態になる
	*            画像index=0に初期化される
	*/
	virtual bool capture() = 0;
	/**
	* @brief HWトリガを発生させる
	* @param[in] mode DLP発光モード
	* @return true:正常, false : 異常
	*/
	virtual bool trigger(DlpMode mode) = 0;
	/**
	* @brief 露光時間取得
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @return 露光時間(us), -1:異常
	*/
	virtual int exposureTime(int camno) = 0;
	/**
	* @brief ゲイン取得
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @return ゲイン,-1:異常
	*/
	virtual int gain(int camno) = 0;
	/**
	* @brief 拡張ゲイン取得
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @return ゲイン,-1:異常
	*/
	virtual int gainx(int camno) = 0;
	/**
	* @brief 露光時間設定
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @param[in] et 露光時間(us)
	* @return true:正常,false:異常
	*/
	virtual bool setExposureTime(int camno, int et) = 0;
	/**
	* @brief ゲイン設定
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @param[in] gn ゲイン
	* @return true:正常,false:異常
	*/
	virtual bool setGain(int camno, int gn) = 0;
	/**
	* @brief 拡張ゲイン設定
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @param[in] gn ゲイン
	* @return true:正常,false:異常
	*/
	virtual bool setGainx(int camno, int gn) = 0;
	/**
	* @brief トリガーモード取得
	* @return トリガーモード
	*/
	virtual TrigMode triggerMode() = 0;
	/**
	* @brief トリガーモード設定
	* @param[in] tm トリガーモード
	* @return true:正常,false:異常
	*/
	virtual bool setTriggerMode(TrigMode tm) = 0;
	/**
	* @brief 画像データ取得時のコールバック関数設定
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @param[in] OnRecvImage コールバック関数
	* @param[in] mem 画像バッファ,必要なサイズ分呼び出し側で確保しておく
	*/
	virtual void addCallbackRecvImage(int camno, RecvImageFunc OnRecvImage, void *mem) = 0;
	/**
	* @brief カメラ切断時のコールバック関数設定
	* @param[in] OnLostDevice コールバック関数
	*/
	virtual void addCallbackLostDevice(DeviceLostFunc OnLostDevice) = 0;
	//////////
	/**
	* @brief シリアルポートを開く
	* @param[in] portNo ポート番号"COM[portNo]"
	* @return true:正常,false:異常
	*/
	virtual bool openComPort(int portNo) = 0;
	/**
	* @brief シリアルポートを閉じる
	*/
	virtual void closeComPort() = 0;
	/**
	* @brief プロジェクターをリセットする
	* @return true:正常,false:異常
	*/
	virtual bool resetProjector() = 0;
	/**
	* @brief プロジェクター照明強度を取得
	* @return 強度,-1:異常
	*/
	virtual int projectorBrightness() = 0;
	/**
	* @brief プロジェクター照明強度を設定
	* @param[in] value 強度(0～255)
	* @return true:正常,false:異常
	*/
	virtual bool setProjectorBrightness(int value) = 0;
	/**
	* @brief プロジェクター固定パターンを設定
	* @param[in] pattern DLPパターン
	* @return true:正常,false:異常
	*/
	virtual bool setProjectorFixedPattern(DlpPattern pattern) = 0;
	/**
	* @brief プロジェクター露光時間を取得
	* @return 露光時間(usec),-1:異常
	* @date 2020-3-17
	*/
	virtual int projectorExposureTime() = 0;
	/**
	* @brief プロジェクター露光時間設定
	* @param[in] et 露光時間(ms)
	* @return true:正常,false:異常
	*/
	virtual bool setProjectorExposureTime(int et) = 0;
	//////////
	/**
	* @brief 一連のシーケンスカウンタをリセットする
	*        コールバック関数indexをリセットする
	*/
	virtual void resetSequence() = 0;
	/**
	* @brief 撮影枚数取得
	* @param[in] camno [0]:左カメラ,[1]:右カメラ
	* @return 撮影枚数
	*/
	virtual int capturedCount(int camno) = 0;

	/**
	* @brief プロジェクター発光間隔を設定
	* @param[in] value 間隔msec
	* @return true:正常,false:異常
	* @date 2020-3-17
	*/
	virtual bool setProjectorFlashInterval(int value) = 0;
	/**
	* @brief プロジェクター発光間隔を取得
	* @param[in] value 間隔msec
	* @return true:正常,false:異常
	* @date 2020-3-17
	*/
	virtual int projectorFlashInterval() = 0;
	/**
	* @brief プリセット番号設定
	* @param[in] no プリセット番号(0～)
	* @return true:正常,false:異常
	* @date 2020-11-16
	*/
	virtual bool setPresetNo(int no) = 0;
	/**
	* @brief プリセット番号取得
	* @return 1～:正常,0:異常
	* @date 2020-11-16
	*/
	virtual int presetNo() = 0;

	/**
	 * @brief トリガ発行時枚数
	 *        trigger(DlpModeCont)実行時の撮影枚数
	 *        初期値=13
	 * @param[in] num 枚数
	 * @date 2020-11-19
	 */
	virtual void setTriggerCount(int num) = 0;

	////////// 汎用I/F
	/**
	* @brief パラメタ名に関連付けられた整数値を設定
	* @param[in] name パラメタ名
	* @param[in] value 整数値
	* @return true:正常,false:異常
	* @date 2020-3-17
	*/
	virtual bool setIntegerValue(const char *name, int value) = 0;
	/**
	* @brief パラメタ名に関連付けられた整数値を取得
	* @param[in] name パラメタ名
	* @param[out] ok true:正常,false:異常
	* @return 整数値
	* @date 2020-3-17
	*/
	virtual int getIntegerValue(const char *name, bool *ok = 0) = 0;
	/**
	* @brief パラメタ名に関連付けられた実数値を設定
	* @param[in] name パラメタ名
	* @param[in] value 実数値
	* @return true:正常,false:異常
	* @date 2020-3-17
	*/
	virtual bool setFloatValue(const char *name, float value) = 0;
	/**
	* @brief パラメタ名に関連付けられた実数値を取得
	* @param[in] name パラメタ名
	* @param[out] ok true:正常,false:異常
	* @return 実数値
	* @date 2020-3-17
	*/
	virtual float getFloatValue(const char *name, bool *ok = 0) = 0;
	/**
	* @brief パラメタ名に関連付けられた文字列値を設定
	* @param[in] name パラメタ名
	* @param[in] value 文字列値
	* @return true:正常,false:異常
	* @date 2020-3-17
	*/
	virtual bool setStringValue(const char *name, const char *value) = 0;
	/**
	* @brief パラメタ名に関連付けられた文字列値を取得
	* @param[in] name パラメタ名
	* @param[out] ok true:正常,false:異常
	* @return 文字列値
	* @date 2020-3-17
	*/
	virtual bool getStringValue(const char *name, char *retbuf, int retbuf_size) = 0;
};

#ifdef _USRDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
/**
* @brief YCAM3Dクラスインスタンス作成関数宣言
*/
extern "C" EXPORT IYCam3D *CreateIYCam3DInstance();
/**
* @brief YCAM3Dクラスインスタンス作成関数ポインタ
*/
typedef IYCam3D *(*pCreateIYCam3DInstance)();
/**
* @brief YCAM3Dクラスインスタンス作成関数名
*/
#define YCAM3DFUNCNAME "CreateIYCam3DInstance"
