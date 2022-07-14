@echo off
:Usage: Calib3D dirname hdrname n_frames yaml_filename
:   dirname : �L�����u�摜�i�[��f�B���N�g����
:   hdrname : �L�����u�摜�t�@�C��������ԍ����Ɗg���q����菜��������
:   n_frames : ���t���[����
:   yaml_filename : �L�����u�p�����[�^
cd /d %~dp0
copy ..\calib\calib00.pgm ..\calib\reference.pgm >calib.txt
calib3d ..\calib calib 8 calib.yaml >calib.txt 2>&1
type calib.txt
del ..\calib\reference.pgm stereoL.map stereoR.map
move calib.txt ..\calib >nul
move cam0_param.yaml ..\calib >nul
move cam1_param.yaml ..\calib >nul
move stereo_param.yaml ..\calib >nul
move hmat0.dat ..\calib >nul
move hmat1.dat ..\calib >nul
move rect.param ..\calib >nul
