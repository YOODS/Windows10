#pragma once
/**
* @brief IPC�w�b�_
*
*/

/**
* @brief �R�}���h���ʎq�̒���
*/
#define CMD_NAME_MAX 20

/**
* @brief ���O�o�͐�
*/
enum IpcLogMode
{
	IpcLogMode_None = 0x00,		// �Ȃ�
	IpcLogMode_File = 0x01,		// �t�@�C��
	IpcLogMode_Console = 0x02,	// �R���\�[��
	IpcLogMode_Debugger = 0x04,	// OutputDebugString
	IpcLogMode_ALL = 0x07		// ���ׂĂ̏o�͐�
};

/**
* @brief �R�}���h�^�C�v
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
* @brief �f�[�^�^
*/
enum IpcDataType
{
	IpcDataType_Int = TEXT('I'),	//int
	IpcDataType_Float = TEXT('F'),	//float
	IpcDataType_Double = TEXT('D'),	//double
	IpcDataType_Data = TEXT('A')	//byte array
};

/**
* @brief Variant�f�[�^
*/
typedef union VAR {
	INT    ival;	// INT�l
	FLOAT  fval;	// FLOAT�l
	DOUBLE dval;	// DOUBLE�l
	INT    data[1];	// �f�[�^�擪�A�h���X
	//
	VAR(){}
	VAR(INT i) : ival(i){}
	VAR(FLOAT f) : fval(f){}
	VAR(DOUBLE d) : dval(d){}
	VAR(TCHAR *s){ TCHAR *d = (TCHAR*)data; while ((*d++ = *s++) != 0); }
} VAR, *PVAR;

/**
* @brief IPC�R�[���o�b�N�N���X
*/
struct IpcCallbackFunc {
	/**
	* @brief IPC�R�[���o�b�N�֐�
	* @param[in] id ���b�Z�[�WID
	* @param[in] cmdType �R�}���h�^�C�v(IpcCmdType)
	* @param[in] cmdName �R�}���h���ʎq
	* @param[in] dataType �f�[�^�^
	* @param[out] pOK BOOL=����,FALSE=�ُ�
	* @param[out] pVar �l
	* @param[out] pSize �l�̃T�C�Y
	*/
	virtual void operator()(CONST USHORT id, CONST CHAR cmdType, LPCSTR cmdName, CONST CHAR dataType,
		LPBOOL pOK, PVAR pVar, LPINT pSize) = 0;
	/**
	* @brief IPC�R�[���o�b�N�֐�����
	* @attention server�ŃR�[���o�b�N����񉻂��鎞�ɔh���N���X�̐���: return new Derived; ���L�q����
	*/
	virtual IpcCallbackFunc *create(){ return 0; }
};

/**
* @brief IPC�T�[�o�[�N���X
*/
class IIpcServer
{
public:
	/**
	* @brief ���O�̐ݒ�
	* @param[in] mode enum LogMode ��OR�l
	*            ����DLL���O�R���\�[���ƕ��p�͕s��
	* @param[in] path LogMode_File�w�莞�̃��O�t�@�C���p�X
	* @attention ���O�t�@�C���͐V�K�쐬�����
	*/
	virtual void setLog(int mode, LPCTSTR path = 0) = 0;
	/**
    * @brief �I������
    */
	virtual void destroy() = 0;
	/**
    * @brief �ڑ���ҋ@����
	* @param[in] pipe_name �ڑ���
	* @param[in] nmax �ڑ��ő吔
	* @return true:����,false:�ُ�
    */
	virtual bool open(LPCTSTR pipe_name, int nmax = 1) = 0;
	/**
	* @brief �ڑ������
	*/
	virtual void close() = 0;
	/**
	* @brief �T�[�o�[�ŗL�̓�������邽�߂̃R�[���o�b�N�֐��o�^
	* @param[in] OnCallback �R�[���o�b�N�֐�
	* @attention �����ɕ��A����R�[�h�ɂ��邱��
	*/
	virtual void addCallbackIpc(IpcCallbackFunc *OnCallback) = 0;
	/**
	* @brief �T�[�o�[����N���C�A���g�Ƀg���K�𔭐�������
	* @param[in] cmdName �R�}���h���ʎq
	* @param[in] dataType �f�[�^�^(IpcDataType)
	* @param[in] var �f�[�^
	* @param[in] size �f�[�^�T�C�Y(IpcDataType_Data�̃T�C�Y,�Œ蒷�^�͖��w���)
	* @attention �N���C�A���g�̓R�[���o�b�N�֐���o�^���Ă��邱��
	*/
	virtual bool trigger(LPCTSTR cmdName, CONST TCHAR dataType, VAR &var, INT size) = 0;
	virtual bool trigger(LPCTSTR cmdName, CONST TCHAR dataType, VAR var) = 0;
	virtual bool trigger(LPCTSTR cmdName) = 0;
	/**
	* @brief �ڑ����Ă���N���C�A���g�̐���Ԃ�
	* @return �ڑ����Ă���N���C�A���g�̐�
	*/
	virtual int clients() = 0;

	/**
	* @brief ���L���������쐬����
	* @param[in] name ���L��������
	* @param[in] size ���L�������T�C�Y(byte)
	* @return ���L�������擪�A�h���X,0:�ُ�
	*/
	virtual LPVOID createShMem(LPCTSTR name, DWORD size) = 0;
	/**
	* @brief ���L���������폜
	* @param[in] pMap ���L�������擪�A�h���X
	*            0:�S�Ă̋��L���������폜
	* @return true:����,false:�ُ�
	*/
	virtual void deleteShMem(LPVOID pMap = 0) = 0;
};

/**
* @brief IPC�N���C�A���g�N���X
*/
class IIpcClient
{
public:
	/**
	* @brief ���O�̐ݒ�
	* @param[in] mode enum LogMode ��OR�l
	*            ����DLL���O�R���\�[���ƕ��p�͕s��
	* @param[in] path LogMode_File�w�莞�̃��O�t�@�C���p�X
	* @attention ���O�t�@�C���͐V�K�쐬�����
	*/
	virtual void setLog(int mode, LPCTSTR path = 0) = 0;
	/**
    * @brief �I������
    */
	virtual void destroy() = 0;
	/**
    * @brief �T�[�o�[�ɐڑ�����
	* @param[in] pipe_name �ڑ���
	* @return true:����,false:�ُ�
    */
	virtual bool open(LPCTSTR pipe_name) = 0;
	/**
	* @brief �ڑ������
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
	* @brief �N���C�A���g�Ŕ񓯊��Ƀf�[�^���󂯎�邽�߂̃R�[���o�b�N�֐��o�^
	* @attention �K�v�Ȃ���Γo�^���Ȃ��Ă悢
	*            �����ɕ��A����R�[�h�ɂ��邱��
	* @param[in] OnCallback �R�[���o�b�N�֐�
	*/
	virtual void addCallbackIpc(IpcCallbackFunc *OnCallback) = 0;
	virtual void delCallbackIpc() = 0;

	/**
	* @brief ���L���������J��
	* @param[in] name ���L��������
	* @return ���L�������擪�A�h���X,0:�ُ�
	*/
	virtual LPVOID openShMem(LPCTSTR name) = 0;
	/**
	* @brief ���L�����������
	* @param[in] pMap ���L�������擪�A�h���X
	*            0:�S�Ă̋��L�����������
	* @return true:����,false:�ُ�
	*/
	virtual void closeShMem(LPVOID pMap = 0) = 0;
};

#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

/**
* @brief IPC�T�[�o�[�N���X�C���X�^���X�쐬�֐��錾
*/
extern "C" EXPORT IIpcServer *CreateIpcServerInstance();
/**
* @brief IPC�T�[�o�[�N���X�C���X�^���X�쐬�֐��|�C���^
*/
typedef IIpcServer *(*pCreateIpcServerInstance)();
/**
* @brief IPC�T�[�o�[�N���X�C���X�^���X�쐬�֐���
*/
#define IPC_SERVER_FUNC_NAME "CreateIpcServerInstance"

/**
* @brief IPC�N���C�A���g�N���X�C���X�^���X�쐬�֐��錾
*/
extern "C" EXPORT IIpcClient *CreateIpcClientInstance();
/**
* @brief IPC�N���C�A���g�N���X�C���X�^���X�쐬�֐��|�C���^
*/
typedef IIpcClient *(*pCreateIpcClientInstance)();
/**
* @brief IPC�N���C�A���g�N���X�C���X�^���X�쐬�֐���
*/
#define IPC_CLIENT_FUNC_NAME "CreateIpcClientInstance"
