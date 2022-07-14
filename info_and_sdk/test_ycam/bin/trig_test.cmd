@echo off
cd /d %~dp0

:接続先
set PIPE=YCAM_IPC

:発光回数
set COUNT=5
:発光トリガ間隔
set TIMEOUT=1

://///////////////////////
:カメラ露光時間[E]
set CAMET=16800
://///////////////////////
:プロジェクター露光時間[X]
set PRJET=100
:プロジェクター露光強度[B]
set PRJINT=255
:プロジェクター発光間隔[W]
set PRJITVL=110
:プロジェクタパターン[N]
set PRJPTN=1
://///////////////////////

echo 回数=%COUNT%
rem echo 間隔=%TIMEOUT%秒

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
rem 次のtimeoutを外すとdisconnectになる
rem    timeout /t %TIMEOUT% > nul
    ipcmd %PIPE% H
    ipcmd %PIPE% T -s -i 2
    timeout /t %TIMEOUT% > nul
)



