@echo off
:Usage: Phsft3D [-hrt] [-f cfgfile] [-c calib_dir] [-s images_dir] [-p pattern]
:               [-o 3d_file]
:  -h       �g������\��
:  -r       ���N�e�B�t�@�C�����s
:  -t       �_�Q���{�[�h���W�n�ɕϊ�.(�A���AboardRT.yaml���L�����u���[�V�����f�B���N�g���ɑ��݂���ꍇ�̂ݗL��. Hmat�ō��W�n���ϊ�����Ă���ꍇ�A�ēx�ړ����邱�ƂɂȂ�̂ŁA�w�肵�Ȃ��悤�ɂ��Ă�������.
:  -f cfgfile       �ݒ�t�@�C����.(�f�t�H���g=./pshft.cfg)
:  -c calib_dir     �L�����u���[�V�����f�B���N�g��(.\)
:  -s images_dir    ���͉摜�f�B���N�g��(.\)
:  -p pattern       ���͉摜�t�@�C�����p�^�[��(*.pgm)
:  -o 3d_file       �o�͓_�Q�t�@�C��(phsft.ply)
:
Phsft3D.exe -r -f PHSFT.ini -c ..\calib -s ..\capt -p *.pgm -o ..\capt\out.ply
