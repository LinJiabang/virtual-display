@echo off
SETLOCAL
IF "%WindowsSDKDir%" == "" (
    echo Invoking VS2015 build environment.
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
)

nmake2msbuild.exe proxy-wddm\kmd\sources -NoPackageProject -NoSolution
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false

REM nmake2msbuild.exe proxy-wddm\test\sources -NoPackageProject -NoSolution
msbuild proxy-wddm\test\ljb_monitor_test.vcxproj /p:Configuration=Release;Platform=Win32

if "%OUTPUT%"== "" set OUTPUT=%CD%\output

if NOT exist %OUTPUT%\package\chk\x86   MKDIR %OUTPUT%\package\chk\x86
if NOT exist %OUTPUT%\package\fre\x86   MKDIR %OUTPUT%\package\fre\x86
if NOT exist %OUTPUT%\package\chk\x64   MKDIR %OUTPUT%\package\chk\x64
if NOT exist %OUTPUT%\package\fre\x64   MKDIR %OUTPUT%\package\fre\x64
if NOT exist %OUTPUT%\pdb\chk\x86       MKDIR %OUTPUT%\pdb\chk\x86
if NOT exist %OUTPUT%\pdb\fre\x86       MKDIR %OUTPUT%\pdb\fre\x86
if NOT exist %OUTPUT%\pdb\chk\x64       MKDIR %OUTPUT%\pdb\chk\x64
if NOT exist %OUTPUT%\pdb\fre\x64       MKDIR %OUTPUT%\pdb\fre\x64
if NOT exist %OUTPUT%\devcon\x86        MKDIR %OUTPUT%\devcon\x86
if NOT exist %OUTPUT%\devcon\x64        MKDIR %OUTPUT%\devcon\x64

REM
REM Prepare package
REM
copy proxy-wddm\kmd\ljb_proxykmd.inf                    %OUTPUT%\package\chk\
stampinf -f                                             %OUTPUT%\package\chk\ljb_proxykmd.inf -v *
copy proxy-wddm\kmd\Debug\x86\ljb_proxykmd.sys          %OUTPUT%\package\chk\x86\
copy proxy-wddm\kmd\Debug\x64\ljb_proxykmd.sys          %OUTPUT%\package\chk\x64\
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %OUTPUT%\package\chk\x86\ljb_umd.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x64\ljb_umd.dll               %OUTPUT%\package\chk\x64\ljb_umd.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %OUTPUT%\package\chk\x86\ljb_umd32.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %OUTPUT%\package\chk\x64\ljb_umd32.dll

copy proxy-wddm\kmd\ljb_proxykmd.inf                    %OUTPUT%\package\fre\
stampinf -f                                             %OUTPUT%\package\fre\ljb_proxykmd.inf -v *
copy proxy-wddm\kmd\Release\x86\ljb_proxykmd.sys        %OUTPUT%\package\fre\x86\
copy proxy-wddm\kmd\Release\x64\ljb_proxykmd.sys        %OUTPUT%\package\fre\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %OUTPUT%\package\fre\x86\ljb_umd.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x64\ljb_umd.dll             %OUTPUT%\package\fre\x64\ljb_umd.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %OUTPUT%\package\fre\x86\ljb_umd32.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.dll             %OUTPUT%\package\fre\x64\ljb_umd32.dll

copy proxy-wddm\test\Release\x86\ljb_monitor_test.exe   %OUTPUT%\package\


REM
REM Prepare PDB
REM
copy proxy-wddm\kmd\Debug\x86\ljb_proxykmd.pdb          %OUTPUT%\pdb\chk\x86\
copy proxy-wddm\kmd\Debug\x64\ljb_proxykmd.pdb          %OUTPUT%\pdb\chk\x64\
copy proxy-wddm\kmd\Release\x86\ljb_proxykmd.pdb        %OUTPUT%\pdb\fre\x86\
copy proxy-wddm\kmd\Release\x64\ljb_proxykmd.pdb        %OUTPUT%\pdb\fre\x64\

REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.pdb               %OUTPUT%\pdb\chk\x86\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x64\ljb_umd.pdb               %OUTPUT%\pdb\chk\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.pdb               %OUTPUT%\pdb\chk\x64\ljb_umd32.pdb
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.pdb             %OUTPUT%\pdb\fre\x86\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x64\ljb_umd.pdb             %OUTPUT%\pdb\fre\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.pdb             %OUTPUT%\pdb\fre\x64\ljb_umd32.pdb

xcopy /s /I /Y testcert %OUTPUT%\testcert
REM
REM run INF2CAT
REM
INF2CAT /uselocaltime /drv:%OUTPUT%\package\chk /os:7_X86,7_X64,8_X86,8_X64,6_3_X86,6_3_X64,10_X86,10_X64
INF2CAT /uselocaltime /drv:%OUTPUT%\package\fre /os:7_X86,7_X64,8_X86,8_X64,6_3_X86,6_3_X64,10_X86,10_X64

set TEST_SIGN=signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll

%TEST_SIGN% %OUTPUT%\package\chk\*.cat
%TEST_SIGN% %OUTPUT%\package\chk\x86\*.sys
%TEST_SIGN% %OUTPUT%\package\chk\x64\*.sys

%TEST_SIGN% %OUTPUT%\package\fre\*.cat
%TEST_SIGN% %OUTPUT%\package\fre\x86\*.sys
%TEST_SIGN% %OUTPUT%\package\fre\x64\*.sys

pushd   virtual_monitor
call    buildall_wdk10.cmd
popd

ENDLOCAL