@echo off
REM nmake2msbuild DIRS -NoSolution -NoPackageProject

SETLOCAL
IF "%WindowsSDKDir%" == "" (
    echo Invoking VS2015 build environment.
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
)

msbuild func\source\vmon_func.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild func\source\vmon_func.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild func\source\vmon_func.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
msbuild func\source\vmon_func.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false

set CWD=%CD%\notify\source
cmd /c "call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\  chk x86 WIN7 & cd /d %CWD% & build -cgz"
cmd /c "call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\  fre x86 WIN7 & cd /d %CWD% & build -cgz"
cmd /c "call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\  chk x64 WIN7 & cd /d %CWD% & build -cgz"
cmd /c "call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\  fre x64 WIN7 & cd /d %CWD% & build -cgz"


if "%OUTPUT%"== "" set OUTPUT=%CD%\output

if NOT exist %OUTPUT%\package\chk\x86       MKDIR %OUTPUT%\package\chk\x86
if NOT exist %OUTPUT%\package\fre\x86       MKDIR %OUTPUT%\package\fre\x86

if NOT exist %OUTPUT%\package\chk\x64       MKDIR %OUTPUT%\package\chk\x64
if NOT exist %OUTPUT%\package\fre\x64       MKDIR %OUTPUT%\package\fre\x64

if NOT exist %OUTPUT%\pdb\chk\x86           MKDIR %OUTPUT%\pdb\chk\x86
if NOT exist %OUTPUT%\pdb\fre\x86           MKDIR %OUTPUT%\pdb\fre\x86

if NOT exist %OUTPUT%\pdb\chk\x64           MKDIR %OUTPUT%\pdb\chk\x64
if NOT exist %OUTPUT%\pdb\fre\x64           MKDIR %OUTPUT%\pdb\fre\x64


REM
REM preparing driver package
REM

copy "%WindowsSdkDir%"Redist\wdf\x86\WdfCoInstaller01011.dll %OUTPUT%\package\chk\x86\
copy "%WindowsSdkDir%"Redist\wdf\x64\WdfCoInstaller01011.dll %OUTPUT%\package\chk\x64\
copy "%WindowsSdkDir%"Redist\wdf\x86\WdfCoInstaller01011.dll %OUTPUT%\package\fre\x86\
copy "%WindowsSdkDir%"Redist\wdf\x64\WdfCoInstaller01011.dll %OUTPUT%\package\fre\x64\

copy func\source\Debug\x86\vmon_func.inf        %OUTPUT%\package\chk\
copy func\source\Release\x86\vmon_func.inf      %OUTPUT%\package\fre\

copy func\source\Debug\x86\vmon_func.sys        %OUTPUT%\package\chk\x86\
copy func\source\Debug\x64\vmon_func.sys        %OUTPUT%\package\chk\x64\
copy func\source\Release\x86\vmon_func.sys      %OUTPUT%\package\fre\x86\
copy func\source\Release\x64\vmon_func.sys      %OUTPUT%\package\fre\x64\

copy notify\source\objchk_win7_x86\i386\notify.exe         %OUTPUT%\package\chk\x86\
copy notify\source\objchk_win7_amd64\amd64\notify.exe      %OUTPUT%\package\chk\x64\
copy notify\source\objfre_win7_x86\i386\notify.exe         %OUTPUT%\package\fre\x86\
copy notify\source\objfre_win7_amd64\amd64\notify.exe      %OUTPUT%\package\fre\x64\

REM
REM preparing PDB
REM
copy func\source\Debug\x86\vmon_func.pdb        %OUTPUT%\pdb\chk\x86\
copy func\source\Debug\x64\vmon_func.pdb        %OUTPUT%\pdb\chk\x64\
copy func\source\Release\x86\vmon_func.pdb      %OUTPUT%\pdb\fre\x86\
copy func\source\Release\x64\vmon_func.pdb      %OUTPUT%\pdb\fre\x64\

copy notify\source\objchk_win7_x86\i386\notify.pdb         %OUTPUT%\pdb\chk\x86\
copy notify\source\objchk_win7_amd64\amd64\notify.pdb      %OUTPUT%\pdb\chk\x64\
copy notify\source\objfre_win7_x86\i386\notify.pdb         %OUTPUT%\pdb\fre\x86\
copy notify\source\objfre_win7_amd64\amd64\notify.pdb      %OUTPUT%\pdb\fre\x64\

REM
REM run INF2CAT
REM
INF2CAT /uselocaltime /drv:%OUTPUT%\package\chk /os:7_X86,7_X64,8_X86,8_X64,10_X86,10_X64
INF2CAT /uselocaltime /drv:%OUTPUT%\package\fre /os:7_X86,7_X64,8_X86,8_X64,10_X86,10_X64

set TIMESTAMP_SERVER=http://timestamp.digicert.com

if "%TEST_SIGN%" == "" set TEST_SIGN=SIGNTOOL sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /ac "d:\cert\DigiCert High Assurance EV Root CA.crt" /i "digicert" /v

REM password : lci53673835
%TEST_SIGN% %OUTPUT%\package\chk\vmon_func.cat %OUTPUT%\package\chk\x86\*.sys %OUTPUT%\package\chk\x86\*.exe %OUTPUT%\package\chk\x64\*.sys %OUTPUT%\package\chk\x64\*.exe
%TEST_SIGN% %OUTPUT%\package\fre\vmon_func.cat %OUTPUT%\package\fre\x86\*.sys %OUTPUT%\package\fre\x86\*.exe %OUTPUT%\package\fre\x64\*.sys %OUTPUT%\package\fre\x64\*.exe
