@echo off
:Usage: Calib3D [-hcodb] [-f cfgfile] [-r number] [-p pattern] calib_dir
:  calib_dir    �L�����u���[�V�����摜�i�[��f�B���N�g��
:               ���E�摜���������Ċi�[����Ă���ꍇ�A0���ڍ��摜, 0���ډE�摜, 1���ڍ��摜...�̏��ɂȂ�悤�t�@�C�����𒲐����Ă�������.
:  -h           �g������\��.
:  -c           ���E�A���摜����͂Ƃ��ė^����.
:  -o           �}�[�J���o���ʂ��L�����u�f�B���N�g���ɕۑ�.(���͉摜�Ɠ������O�Ŋg���q��png�ɒu���������`���ŕۑ����܂�.)
:  -d           �}�[�J���o���ʂ���ʂɕ\������.
:  -r number    �Q�ƃt���[���ԍ�(�f�t�H���g=0).���̃I�v�V�������w�肷��ƃ{�[�h���W�n�ɕϊ�����(Hmat�ɑ΂��Ă̂�)
:  -f cfgfile   �ݒ�t�@�C����.(�f�t�H���g=./calib.cfg)
:  -p pattern   ���͉摜�t�@�C�����p�^�[��(*.pgm)
:
D:\VS_prj\YCAM3D.v3\YdsDec3D20200615\x64\Release\Calib3D.exe -o -f .\calib.cfg -p pgm ..\calib
