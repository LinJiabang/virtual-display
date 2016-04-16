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

if "%BIN%"== "" set BIN=.\bin

if NOT exist %BIN%\package\chk\x86   MKDIR %BIN%\package\chk\x86
if NOT exist %BIN%\package\fre\x86   MKDIR %BIN%\package\fre\x86
if NOT exist %BIN%\package\chk\x64   MKDIR %BIN%\package\chk\x64
if NOT exist %BIN%\package\fre\x64   MKDIR %BIN%\package\fre\x64
if NOT exist %BIN%\pdb\chk\x86       MKDIR %BIN%\pdb\chk\x86
if NOT exist %BIN%\pdb\fre\x86       MKDIR %BIN%\pdb\fre\x86
if NOT exist %BIN%\pdb\chk\x64       MKDIR %BIN%\pdb\chk\x64
if NOT exist %BIN%\pdb\fre\x64       MKDIR %BIN%\pdb\fre\x64
if NOT exist %BIN%\devcon\x86        MKDIR %BIN%\devcon\x86
if NOT exist %BIN%\devcon\x64        MKDIR %BIN%\devcon\x64

REM
REM Prepare package
REM
copy proxy-wddm\kmd\ljb_proxykmd.inf                    %BIN%\package\chk\
stampinf -f                                             %BIN%\package\chk\ljb_proxykmd.inf -v *
copy proxy-wddm\kmd\Debug\x86\ljb_proxykmd.sys          %BIN%\package\chk\x86\
copy proxy-wddm\kmd\Debug\x64\ljb_proxykmd.sys          %BIN%\package\chk\x64\
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %BIN%\package\chk\x86\ljb_umd.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x64\ljb_umd.dll               %BIN%\package\chk\x64\ljb_umd.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %BIN%\package\chk\x86\ljb_umd32.dll
REM NOT YET_IMPLMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %BIN%\package\chk\x64\ljb_umd32.dll

copy proxy-wddm\kmd\ljb_proxykmd.inf                    %BIN%\package\fre\
stampinf -f                                             %BIN%\package\fre\ljb_proxykmd.inf -v *
copy proxy-wddm\kmd\Release\x86\ljb_proxykmd.sys        %BIN%\package\fre\x86\
copy proxy-wddm\kmd\Release\x64\ljb_proxykmd.sys        %BIN%\package\fre\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %BIN%\package\fre\x86\ljb_umd.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x64\ljb_umd.dll             %BIN%\package\fre\x64\ljb_umd.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.dll               %BIN%\package\fre\x86\ljb_umd32.dll
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.dll             %BIN%\package\fre\x64\ljb_umd32.dll

copy proxy-wddm\test\Release\x86\ljb_monitor_test.exe   %BIN%\package\


REM
REM Prepare PDB
REM
copy proxy-wddm\kmd\Debug\x86\ljb_proxykmd.pdb          %BIN%\pdb\chk\x86\
copy proxy-wddm\kmd\Debug\x64\ljb_proxykmd.pdb          %BIN%\pdb\chk\x64\
copy proxy-wddm\kmd\Release\x86\ljb_proxykmd.pdb        %BIN%\pdb\fre\x86\
copy proxy-wddm\kmd\Release\x64\ljb_proxykmd.pdb        %BIN%\pdb\fre\x64\

REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.pdb               %BIN%\pdb\chk\x86\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x64\ljb_umd.pdb               %BIN%\pdb\chk\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Debug\x86\ljb_umd.pdb               %BIN%\pdb\chk\x64\ljb_umd32.pdb
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.pdb             %BIN%\pdb\fre\x86\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x64\ljb_umd.pdb             %BIN%\pdb\fre\x64\
REM NOT_YET_IMPLEMENTED copy proxy-wddm\umd\Release\x86\ljb_umd.pdb             %BIN%\pdb\fre\x64\ljb_umd32.pdb

xcopy /s /I /Y testcert %BIN%\testcert
REM
REM run INF2CAT
REM
INF2CAT /uselocaltime /drv:%BIN%\package\chk /os:7_X86,7_X64,8_X86,8_X64,6_3_X86,6_3_X64,10_X86,10_X64
INF2CAT /uselocaltime /drv:%BIN%\package\fre /os:7_X86,7_X64,8_X86,8_X64,6_3_X86,6_3_X64,10_X86,10_X64

signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\chk\*.cat
signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\chk\x86\*.sys
REM signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\chk\x86\*.dll
signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\chk\x64\*.sys
REM signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\chk\x64\*.dll

signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\fre\*.cat
signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\fre\x86\*.sys
REM signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\fre\x86\*.dll
signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\fre\x64\*.sys
REM signtool sign /v /s PrivateCertStore /n LJB(Test) /t http://timestamp.verisign.com/scripts/timstamp.dll %BIN%\package\fre\x64\*.dll

ENDLOCAL