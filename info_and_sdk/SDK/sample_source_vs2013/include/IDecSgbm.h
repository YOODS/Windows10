#pragma once
/**
* �X�e���I���֊֘A�w�b�_
*
*/

/**
* @enum LogMode
* @brief ���O�o�͐�
*/
enum SgbmLogMode
{
	SgbmLogMode_None = 0x00,		/// �Ȃ�
	SgbmLogMode_File = 0x01,		/// �t�@�C��
	SgbmLogMode_Console = 0x02,		/// �R���\�[��
	SgbmLogMode_Debugger = 0x04,	/// OutputDebugString
	SgbmLogMode_ALL = 0x07			/// ���ׂĂ̏o�͐�
};


/**
* @brief �X�e���I���� 3D�����N���X
*/
class IDecSgbm
{
public:
	/**
    * @brief �I������
    */
	virtual void destroy() = 0;
	/**
	* @brief ��������
	* @param[in] inipath INI�t�@�C���p�X
	* @return true:����,false:�ُ�
    */
	virtual bool init(LPCSTR inipath = 0) = 0;
	/**
	* @brief �ˉe�ϊ��s��ݒ�(�t�@�C������)
	* @param[in] dir hmat0.dat,hmat1.dat������f�B���N�g��(�v\�I�[)
	* @return true:����,false:�ُ�
    */
	virtual bool setHmat(LPCSTR dir) = 0;
	/**
	* @brief �ˉe�ϊ��s��ݒ�(�����ݒ�)
	* @param[in] mtx0[][4] ��-3*4�s��
	* @param[in] mtx1[][4] �E-3*4�s��
    */
	virtual void setHmat(double mtx0[][4], double mtx1[][4]) = 0;
	/**
    * @brief ���N�e�B�t�@�C�p�����[�^�ݒ�
	*        (���N�e�B�t�@�C�摜����͂Ƃ���ꍇ�͕s�v)
	* @param[in] dir rect.param������f�B���N�g��(�v\�I�[)
	* @return true:����,false:�ُ�
    */
	virtual bool setRectParam(LPCSTR dir) = 0;
	/**
    * @brief �J�����摜�ݒ�
	*        (���N�e�B�t�@�C�摜����͂Ƃ���ꍇ�͕s�v)
	* @param[in] ptr[] [0]:���摜�`�����N,[1]:�E�摜�`�����N
	* @param[in] color [false]:���m�N��,[true]:�E�J���[
	*/
	virtual void setCameraImages(BYTE *ptr[], bool color = false) = 0;
	/**
    * @brief ���N�e�B�t�@�C�摜�ݒ�
	*        (�J�����摜����͂Ƃ���ꍇ�͕s�v)
	* @param[in] ptr[] [0]:���摜,[1]:�E�摜
	*                  ���N�e�B�t�@�C�摜�o�b�t�@�����m�ۂ���ꍇ�͖��w��(=0)
    */
	virtual void setRectifiedImages(BYTE *ptr[]) = 0;
	/**
	* @brief ���N�e�B�t�@�C�����s����
	* @param[in] camera_index [0]:���摜,[1]:�E�摜
	* @param[in] index �摜index(-1,0)
	* @param[in/out] camera_image �J�����摜�o�b�t�@�AsetCameraImages�Ŋm�ۂ����o�b�t�@�����ɂ��Ȃ��ꍇ�Ɏw�肷��(index=-1)
	*/
	virtual void rectify(int camera_index, int index, BYTE *camera_image) = 0;
	/**
    * @brief ���N�e�B�t�@�C���s
	* @param[out] xyz ���W�z��(x,y,z,x,y,z,x,....,y,z)
	*                 �A�v���P�[�V��������J�����Ȃ�����
	* @param[out] rgb �F�z��(r,g,b,r,g,b,r,....,g,b)
	*                 �A�v���P�[�V��������J�����Ȃ�����
	* @param[in] do_rectify true:���N�e�B�t�@�C�����s���ĕ���
	                        false:���N�e�B�t�@�C���s��������
	* @return �������ꂽ���_���A�ُ펞��0���Ԃ�
    */
	virtual int exec(FLOAT **xyz = 0, BYTE **rgb = 0, bool do_rectify = true, INT **rg_id = 0) = 0;
	/**
	* @brief ���O�̐ݒ�
	* @param[in] mode enum LogMode ��OR�l
	*            ����DLL���O�R���\�[���ƕ��p�͕s��
	* @param[in] path LogMode_File�w�莞�̃��O�t�@�C���p�X
	* @attention ���O�t�@�C���͐V�K�쐬�����
	*/
	virtual void setLog(int mode, LPCSTR path = 0) = 0;
	///////////////////////////////////////////////////////////////////////////
	/**
    * @brief ���N�e�B�t�@�C�摜�o�b�t�@�A�h���X�擾
	* @param[in] lr 0:��,1:�E
	* @return ���N�e�B�t�@�C�摜�o�b�t�@�A�h���X
    */
	virtual BYTE *rectified_image(int lr) = 0;
	/**
    * @brief ���_���擾
	* @return �������ꂽ���_��
    */
	virtual int data_count() = 0;
	/**
    * @brief ���W�o�b�t�@�A�h���X�擾
	* @return ���W�o�b�t�@�A�h���X
    */
	virtual float *xyz() = 0;
	/**
    * @brief �F�o�b�t�@�A�h���X�擾
	* @return �F�o�b�t�@�A�h���X
    */
	virtual BYTE *rgb() = 0;
	/**
    * @brief �摜���擾
	* @return 1�摜�̕�
    */
	virtual int width() = 0;
	/**
    * @brief �摜�����擾
	* @return 1�摜�̍���
    */
	virtual int height() = 0;
	/**
	* @brief �����W�O���b�h�o�b�t�@�A�h���X�擾
	* @return �����W�O���b�h�o�b�t�@�A�h���X
	*/
	virtual INT *rg_id() = 0;
};


#ifdef _USRDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
/**
* @brief �ʑ��V�t�g3D�����N���X�C���X�^���X�쐬�֐��錾
*/
extern "C" EXPORT IDecSgbm *CreateIDecSgbmInstance();
/**
* @brief �ʑ��V�t�g3D�����N���X�C���X�^���X�쐬�֐��|�C���^
*/
typedef IDecSgbm *(*pCreateIDecSgbmInstance)();
