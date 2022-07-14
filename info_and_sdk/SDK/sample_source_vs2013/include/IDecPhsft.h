#pragma once
/**
* 位相シフト関連ヘッダ
*
*/

/**
* @enum LogMode
* @brief ログ出力先
*/
enum PhsftLogMode
{
	PhsftLogMode_None = 0x00,		/// なし
	PhsftLogMode_File = 0x01,		/// ファイル
	PhsftLogMode_Console = 0x02,	/// コンソール
	PhsftLogMode_Debugger = 0x04,	/// OutputDebugString
	PhsftLogMode_ALL = 0x07			/// すべての出力先
};


/**
* @brief 位相シフト3D復元クラス
*/
class IDecPhsft
{
public:
	/**
    * @brief 終了処理
    */
	virtual void destroy() = 0;
	/**
	* @brief 初期処理
	* @param[in] inipath INIファイルパス
	* @return true:正常,false:異常
    */
	virtual bool init(LPCSTR inipath = 0) = 0;
	/**
	* @brief 射影変換行列設定(ファイルから)
	* @param[in] dir hmat0.dat,hmat1.datがあるディレクトリ(要\終端)
	* @return true:正常,false:異常
    */
	virtual bool setHmat(LPCSTR dir) = 0;
	/**
	* @brief 射影変換行列設定(引数設定)
	* @param[in] mtx0[][4] 左-3*4行列
	* @param[in] mtx1[][4] 右-3*4行列
    */
	virtual void setHmat(double mtx0[][4], double mtx1[][4]) = 0;
	/**
    * @brief レクティファイパラメータ設定
	*        (レクティファイ画像を入力とする場合は不要)
	* @param[in] dir rect.paramがあるディレクトリ(要\終端)
	* @return true:正常,false:異常
    */
	virtual bool setRectParam(LPCSTR dir) = 0;

	/**
	 * 画像読み込み前の初期化を行います.
	 * @return true:正常,false:異常
	 */
	virtual bool reset() = 0;

	/**
    * @brief カメラ画像設定
	*        (レクティファイ画像を入力とする場合は不要)
	* @param[in] ptr[] [0]:左画像チャンク,[1]:右画像チャンク
    */
	virtual void setCameraImages(BYTE *ptr[]) = 0;
	/**
    * @brief レクティファイ画像設定
	*        (カメラ画像を入力とする場合は不要)
	* @param[in] ptr[] [0]:左画像チャンク,[1]:右画像チャンク
	*                  レクティファイ画像バッファだけ確保する場合は無指定(=0)
	* @return レクティファイ画像チャンク先頭アドレス
    */
	virtual BYTE *allocRectifiedImages(BYTE *ptr[] = 0) = 0;
	/**
	* @brief レクティファイを実行する
	* @param[in] camera_index [0]:左画像,[1]:右画像
	* @param[in] index 画像index(0〜)
	* @param[in] camera_image カメラ画像バッファ、setCameraImagesで確保したバッファを元にしない場合に指定する
	*/
	virtual void rectify(int camera_index, int index, BYTE *camera_image = 0) = 0;
	/**
    * @brief レクティファイ実行
	* @param[out] xyz 座標配列(x,y,z,x,y,z,x,....,y,z)
	*                 アプリケーションから開放しないこと
	* @param[out] rgb 色配列(r,g,b,r,g,b,r,....,g,b)
	*                 アプリケーションから開放しないこと
	* @param[in] do_rectify true:レクティファイを実行して復元
	                        false:レクティファイ実行せず復元
	* @param[out] rg_id レンジグリッド頂点のピクセルインデックス配列
	*                     アプリケーションから開放しないこと
	* @return 復元された頂点数、異常時は0が返る
	*/
	virtual int exec(FLOAT **xyz = 0, BYTE **rgb = 0, bool do_rectify = true, INT **rg_id = 0) = 0;
	/**
	* @brief ログの設定
	* @param[in] mode enum LogMode のOR値
	*            他のDLLログコンソールと併用は不可
	* @param[in] path LogMode_File指定時のログファイルパス
	* @attention ログファイルは新規作成される
	*/
	virtual void setLog(int mode, LPCSTR path = 0) = 0;
	///////////////////////////////////////////////////////////////////////////
	/**
    * @brief レクティファイ画像バッファアドレス取得
	* @param[in] camera_index [0]:左画像,[1]:右画像
	* @param[in] index 画像index(0〜)
	* @return レクティファイ画像バッファアドレス
	*/
	virtual BYTE *rbuf(int camera_index = 0, int index = 0) = 0;
	/**
    * @brief 頂点数取得
	* @return 復元された頂点数
    */
	virtual int data_count() = 0;
	/**
    * @brief 座標バッファアドレス取得
	* @return 座標バッファアドレス
    */
	virtual float *xyz() = 0;
	/**
    * @brief 色バッファアドレス取得
	* @return 色バッファアドレス
    */
	virtual BYTE *rgb() = 0;
	/**
    * @brief 画像幅取得
	* @return 1画像の幅
    */
	virtual int width() = 0;
	/**
    * @brief 画像高さ取得
	* @return 1画像の高さ
    */
	virtual int height() = 0;
	/**
    * @brief 位相パターン数取得
	* @return 位相パターン数
    */
	virtual int ptn_num() = 0;
	/**
	* @brief カラーイメージ設定
	* @return なし
	*/
	virtual void setColor(BYTE* cdat)=0;
	/**
	* @brief レンジグリッドバッファアドレス取得
	* @return レンジグリッドバッファアドレス
	*/
	virtual INT *rg_id() = 0;
};


#ifdef _USRDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
/**
* @brief 位相シフト3D復元クラスインスタンス作成関数宣言
*/
extern "C" EXPORT IDecPhsft *CreateIDecPhsftInstance();
/**
* @brief 位相シフト3D復元クラスインスタンス作成関数ポインタ
*/
typedef IDecPhsft *(*pCreateIDecPhsftInstance)();
