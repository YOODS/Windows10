@echo off
setlocal enabledelayedexpansion
cd /d %~dp0


set PRESET=5
set /a PRESET += 1
if 5 lss !PRESET! (
  set PRESET = 1
)
echo PRESET=!PRESET!
