#pragma once
/**
* @brief YCAM3D�w�b�_
*
*/

/**
* @brief �J������
*/
#define CAMNUM 2

/**
* @brief ���O�o�͐�
*/
enum CamLogMode
{
	CamLogMode_None = 0x00,		/// �Ȃ�
	CamLogMode_File = 0x01,		/// �t�@�C��
	CamLogMode_Console = 0x02,	/// �R���\�[��
	CamLogMode_Debugger = 0x04,	/// OutputDebugString
	CamLogMode_ALL = 0x07		/// ���ׂĂ̏o�͐�
};

/**
* @brief �g���K�[���[�h
*/
enum TrigMode
{
	TrigMode_HW,	/// HW�g���K
	TrigMode_SW		/// SW�g���K
};

/**
* @brief DLP�������[�h
*/
enum DlpMode
{
	DlpModeOnce,		///�V���O������(�L���v�`���Ȃ� / X���ڂ̃p�^�����Ǝ�; X = �v���W�F�N�^�[�Œ�p�^�[��)
	DlpModeCapture,		///�V���O������(�L���v�`������ / 1���ڂ̃p�^�����Ǝ�)
	DlpModeCont			///�A������(�L���v�`������ / 1����K�萔�̃p�^����A���Ǝ�)
};

/**
* @brief DLP�p�^�[��
*/
enum DlpPattern
{
	DlpPattFixed,	//�Œ�p�^�[��
	DlpPattPhsft,	//�ʑ��V�t�g�p
	DlpPattStrobe,	//�X�g���{�B�e�p
	DlpPattFocus,	//�s���g���킹�p
	DlpPattStereo,	//���֗p
	DlpPattPhsft3	//�ʑ��V�t�g�p(3�ʑ���)
};

/**
* @brief �摜�f�[�^�R�[���o�b�N�֐��^
* @param[out] camno [0]:���J����,[1]:�E�J����
* @param[out] index �摜�C���f�b�N�X(0�`)
* @param[out] width ��
* @param[out] height ����
* @param[out] mem �f�[�^�A�h���X
*/
typedef int (CALLBACK *RecvImageFunc)(int camno, int index, int width, int height, void *mem);

class IYCam3D;
/**
* @brief �J�����ؒf�R�[���o�b�N�֐��^
* @param[out] IYCam3D �J�����N���X
*/
typedef int (CALLBACK *DeviceLostFunc)(IYCam3D *dev);

/**
* @brief YCAM3D�N���X
*/
class IYCam3D
{
public:
	/**
	* @brief ���O�̐ݒ�
	* @param[in] mode enum LogMode ��OR�l
	*            ����DLL���O�R���\�[���ƕ��p�͕s��
	* @param[in] path LogMode_File�w�莞�̃��O�t�@�C���p�X
	* @attention ���O�t�@�C���͐V�K�쐬�����
	*/
	virtual void setLog(int mode, LPCSTR path = 0) = 0;
	/**
    * @brief �I������
    */
	virtual void destroy() = 0;
	/**
    * @brief �J�������J��
	* @return !0:����,0:�ُ�
	*          1:���J����OK,2:�E�J����OK�̘_���a
    */
	virtual int open(int,int) = 0;
	/**
	* @brief �J�������J����Ă��邩
	* @return true:��,false:��
	*/
	virtual bool isOpened() = 0;
	/**
	* @brief �J���������
	* @return true:����,false:�ُ�
	*/
	virtual void close() = 0;
	/**
	* @brief �摜�T�C�Y
	* @return cx:��,cy:����
	*/
	virtual SIZE imageSize() = 0;
	/**
	* @brief SW/HW�g���K�ɂ��B�e
	* @return true:����,false:�ُ�
	* @attention HW�g���K�̏ꍇ�͑҂���ԂɂȂ�
	*            �摜index=0�ɏ����������
	*/
	virtual bool capture() = 0;
	/**
	* @brief HW�g���K�𔭐�������
	* @param[in] mode DLP�������[�h
	* @return true:����, false : �ُ�
	*/
	virtual bool trigger(DlpMode mode) = 0;
	/**
	* @brief �I�����Ԏ擾
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @return �I������(us), -1:�ُ�
	*/
	virtual int exposureTime(int camno) = 0;
	/**
	* @brief �Q�C���擾
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @return �Q�C��,-1:�ُ�
	*/
	virtual int gain(int camno) = 0;
	/**
	* @brief �g���Q�C���擾
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @return �Q�C��,-1:�ُ�
	*/
	virtual int gainx(int camno) = 0;
	/**
	* @brief �I�����Ԑݒ�
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @param[in] et �I������(us)
	* @return true:����,false:�ُ�
	*/
	virtual bool setExposureTime(int camno, int et) = 0;
	/**
	* @brief �Q�C���ݒ�
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @param[in] gn �Q�C��
	* @return true:����,false:�ُ�
	*/
	virtual bool setGain(int camno, int gn) = 0;
	/**
	* @brief �g���Q�C���ݒ�
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @param[in] gn �Q�C��
	* @return true:����,false:�ُ�
	*/
	virtual bool setGainx(int camno, int gn) = 0;
	/**
	* @brief �g���K�[���[�h�擾
	* @return �g���K�[���[�h
	*/
	virtual TrigMode triggerMode() = 0;
	/**
	* @brief �g���K�[���[�h�ݒ�
	* @param[in] tm �g���K�[���[�h
	* @return true:����,false:�ُ�
	*/
	virtual bool setTriggerMode(TrigMode tm) = 0;
	/**
	* @brief �摜�f�[�^�擾���̃R�[���o�b�N�֐��ݒ�
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @param[in] OnRecvImage �R�[���o�b�N�֐�
	* @param[in] mem �摜�o�b�t�@,�K�v�ȃT�C�Y���Ăяo�����Ŋm�ۂ��Ă���
	*/
	virtual void addCallbackRecvImage(int camno, RecvImageFunc OnRecvImage, void *mem) = 0;
	/**
	* @brief �J�����ؒf���̃R�[���o�b�N�֐��ݒ�
	* @param[in] OnLostDevice �R�[���o�b�N�֐�
	*/
	virtual void addCallbackLostDevice(DeviceLostFunc OnLostDevice) = 0;
	//////////
	/**
	* @brief �V���A���|�[�g���J��
	* @param[in] portNo �|�[�g�ԍ�"COM[portNo]"
	* @return true:����,false:�ُ�
	*/
	virtual bool openComPort(int portNo) = 0;
	/**
	* @brief �V���A���|�[�g�����
	*/
	virtual void closeComPort() = 0;
	/**
	* @brief �v���W�F�N�^�[�����Z�b�g����
	* @return true:����,false:�ُ�
	*/
	virtual bool resetProjector() = 0;
	/**
	* @brief �v���W�F�N�^�[�Ɩ����x���擾
	* @return ���x,-1:�ُ�
	*/
	virtual int projectorBrightness() = 0;
	/**
	* @brief �v���W�F�N�^�[�Ɩ����x��ݒ�
	* @param[in] value ���x(0�`255)
	* @return true:����,false:�ُ�
	*/
	virtual bool setProjectorBrightness(int value) = 0;
	/**
	* @brief �v���W�F�N�^�[�Œ�p�^�[����ݒ�
	* @param[in] pattern DLP�p�^�[��
	* @return true:����,false:�ُ�
	*/
	virtual bool setProjectorFixedPattern(DlpPattern pattern) = 0;
	/**
	* @brief �v���W�F�N�^�[�I�����Ԃ��擾
	* @return �I������(usec),-1:�ُ�
	* @date 2020-3-17
	*/
	virtual int projectorExposureTime() = 0;
	/**
	* @brief �v���W�F�N�^�[�I�����Ԑݒ�
	* @param[in] et �I������(ms)
	* @return true:����,false:�ُ�
	*/
	virtual bool setProjectorExposureTime(int et) = 0;
	//////////
	/**
	* @brief ��A�̃V�[�P���X�J�E���^�����Z�b�g����
	*        �R�[���o�b�N�֐�index�����Z�b�g����
	*/
	virtual void resetSequence() = 0;
	/**
	* @brief �B�e�����擾
	* @param[in] camno [0]:���J����,[1]:�E�J����
	* @return �B�e����
	*/
	virtual int capturedCount(int camno) = 0;

	/**
	* @brief �v���W�F�N�^�[�����Ԋu��ݒ�
	* @param[in] value �Ԋumsec
	* @return true:����,false:�ُ�
	* @date 2020-3-17
	*/
	virtual bool setProjectorFlashInterval(int value) = 0;
	/**
	* @brief �v���W�F�N�^�[�����Ԋu���擾
	* @param[in] value �Ԋumsec
	* @return true:����,false:�ُ�
	* @date 2020-3-17
	*/
	virtual int projectorFlashInterval() = 0;
	/**
	* @brief �v���Z�b�g�ԍ��ݒ�
	* @param[in] no �v���Z�b�g�ԍ�(0�`)
	* @return true:����,false:�ُ�
	* @date 2020-11-16
	*/
	virtual bool setPresetNo(int no) = 0;
	/**
	* @brief �v���Z�b�g�ԍ��擾
	* @return 1�`:����,0:�ُ�
	* @date 2020-11-16
	*/
	virtual int presetNo() = 0;

	/**
	 * @brief �g���K���s������
	 *        trigger(DlpModeCont)���s���̎B�e����
	 *        �����l=13
	 * @param[in] num ����
	 * @date 2020-11-19
	 */
	virtual void setTriggerCount(int num) = 0;

	////////// �ėpI/F
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ�����l��ݒ�
	* @param[in] name �p�����^��
	* @param[in] value �����l
	* @return true:����,false:�ُ�
	* @date 2020-3-17
	*/
	virtual bool setIntegerValue(const char *name, int value) = 0;
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ�����l���擾
	* @param[in] name �p�����^��
	* @param[out] ok true:����,false:�ُ�
	* @return �����l
	* @date 2020-3-17
	*/
	virtual int getIntegerValue(const char *name, bool *ok = 0) = 0;
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ�����l��ݒ�
	* @param[in] name �p�����^��
	* @param[in] value �����l
	* @return true:����,false:�ُ�
	* @date 2020-3-17
	*/
	virtual bool setFloatValue(const char *name, float value) = 0;
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ�����l���擾
	* @param[in] name �p�����^��
	* @param[out] ok true:����,false:�ُ�
	* @return �����l
	* @date 2020-3-17
	*/
	virtual float getFloatValue(const char *name, bool *ok = 0) = 0;
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ������l��ݒ�
	* @param[in] name �p�����^��
	* @param[in] value ������l
	* @return true:����,false:�ُ�
	* @date 2020-3-17
	*/
	virtual bool setStringValue(const char *name, const char *value) = 0;
	/**
	* @brief �p�����^���Ɋ֘A�t����ꂽ������l���擾
	* @param[in] name �p�����^��
	* @param[out] ok true:����,false:�ُ�
	* @return ������l
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
* @brief YCAM3D�N���X�C���X�^���X�쐬�֐��錾
*/
extern "C" EXPORT IYCam3D *CreateIYCam3DInstance();
/**
* @brief YCAM3D�N���X�C���X�^���X�쐬�֐��|�C���^
*/
typedef IYCam3D *(*pCreateIYCam3DInstance)();
/**
* @brief YCAM3D�N���X�C���X�^���X�쐬�֐���
*/
#define YCAM3DFUNCNAME "CreateIYCam3DInstance"
