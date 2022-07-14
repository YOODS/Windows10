@echo off
set TM=0
for /L %%A in (1,1,1000) do (
:	echo %%A && timeout /t %TM% && ipcmd ycam_ipc STATUS -l
	echo %%A && ipcmd ycam_ipc STATUS
)
