@echo off
SETLOCAL enabledelayedexpansion
cd /d %~dp0

set LAST_NO=4

:loop

echo wait trigger...

ipcmd PICK_IPC TRIGGER -t
set no=!errorlevel!
if !no! lss 0 (
	echo get !no! end
    goto end
)


if  !no! lss 1000 (
	
	echo [capture work] no=!no!
	
	set calib_no_str=000!no!
	set calib_no_str=!calib_no_str:~-2!
	set bcalib_dir=..\calib\bcalib_!calib_no_str!
	
	copy /Y !bcalib_dir!\*.* ..\calib\
	echo board calib copied. bcalib_dir=!bcalib_dir!
	
	ipcmd YCAM_IPC TRIGMODE -s -i0
	if not !errorlevel! == 0 (
	    goto end
	)

	ipcmd YCAM_IPC T -s -i2
	if not !errorlevel! == 0 (
	    goto end
	)

	ipcmd YCAM_IPC F -c
	if not !errorlevel! == 0 (
	    goto end
	)

	set file_no_str=000!no!
	set file_no_str=!file_no_str:~-2!
	echo file_no_str = !file_no_str!
	
	copy /Y ..\capt\out.ply ..\capt\out!file_no_str!.ply
	del ..\capt\out.ply

	if !no! == !LAST_NO! (
	    echo ### merge process ###
	    RobotPlyMerge.exe
	)
) else (
	
	set /A calib_no = no - 1000
	echo [board calibration] calib_no=!calib_no!
	
	set calib_no_str=000!calib_no!
	set calib_no_str=!calib_no_str:~-2!
	set bcalib_dir=..\calib\bcalib_!calib_no_str!
	set bcalib_dir=..\calib\bcalib_!calib_no_str!

	ipcmd YCAM_IPC C -s i0
	if not !errorlevel! == 0 (
	    goto end
	)
	ipcmd YCAM_IPC K -c
	if not !errorlevel! == 0 (
		goto end
	)
	mkdir !bcalib_dir! > NUL 2>&1
	copy /Y ..\calib\*.dat !bcalib_dir!
	copy /Y ..\calib\*.param !bcalib_dir!
)

goto loop

:end

echo error occured
