@echo off
cd /d %~dp0

:�ڑ���
set PIPE=YCAM_IPC

:������
set COUNT=5
:�����g���K�Ԋu
set TIMEOUT=1

://///////////////////////
:�J�����I������[E]
set CAMET=16800
://///////////////////////
:�v���W�F�N�^�[�I������[X]
set PRJET=100
:�v���W�F�N�^�[�I�����x[B]
set PRJINT=255
:�v���W�F�N�^�[�����Ԋu[W]
set PRJITVL=110
:�v���W�F�N�^�p�^�[��[N]
set PRJPTN=1
://///////////////////////

echo ��=%COUNT%
rem echo �Ԋu=%TIMEOUT%�b

rem ipcmd %PIPE% E -s -i %CAMET%
rem ipcmd %PIPE% X -s -i %PRJET%
rem ipcmd %PIPE% B -s -i %PRJINT%
rem ipcmd %PIPE% W -s -i %PRJITVL%
rem ipcmd %PIPE% N -s -i %PRJPTN%

rem for /L %%i in (1,1,20) do (
rem     echo ### %%i
rem     ipcmd %PIPE% S
rem     ipcmd %PIPE% H
rem )

rem ipcmd %PIPE% S
rem ipcmd %PIPE% L -s -i 1
rem timeout /t 10

for /L %%i in (1,1,%COUNT%) do (
    echo ### %%i
    ipcmd %PIPE% S
    ipcmd %PIPE% L -s -i 1
	ipcmd %PIPE% E -s -i %CAMET%
    echo start live
    timeout /t 2 > nul
	ipcmd %PIPE% E -s -i %CAMET%
    echo stop live
    ipcmd %PIPE% L -s -i 0
rem ����timeout���O����disconnect�ɂȂ�
rem    timeout /t %TIMEOUT% > nul
    ipcmd %PIPE% H
    ipcmd %PIPE% T -s -i 2
    timeout /t %TIMEOUT% > nul
)



