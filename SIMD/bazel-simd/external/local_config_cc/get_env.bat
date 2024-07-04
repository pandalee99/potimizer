@echo off
call "D:\Visual Studio\Visual Studio 2022\VC\Auxiliary\Build\VCVARSALL.BAT" amd64  -vcvars_ver=14.35.32215 > NUL 
echo PATH=%PATH%,INCLUDE=%INCLUDE%,LIB=%LIB%,WINDOWSSDKDIR=%WINDOWSSDKDIR% 
