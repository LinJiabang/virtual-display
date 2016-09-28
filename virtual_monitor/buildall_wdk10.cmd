@echo off
nmake2msbuild DIRS -NoSolution -NoPackageProject

REM msbuild bus\source\vmon_bus.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild bus\source\vmon_bus.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild bus\source\vmon_bus.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild bus\source\vmon_bus.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
REM 
REM msbuild func\source\vmon_func.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild func\source\vmon_func.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild func\source\vmon_func.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
REM msbuild func\source\vmon_func.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false

msbuild notify\source\notify.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild notify\source\notify.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7;EnableInf2cat=false
msbuild notify\source\notify.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false
msbuild notify\source\notify.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7;EnableInf2cat=false

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

copy bus\source\Debug\x86\vmon_bus.inf          %OUTPUT%\package\chk\
copy bus\source\Release\x86\vmon_bus.inf        %OUTPUT%\package\fre\

copy bus\source\Debug\x86\vmon_bus.sys          %OUTPUT%\package\chk\x86\
copy bus\source\Debug\x64\vmon_bus.sys          %OUTPUT%\package\chk\x64\
copy bus\source\Release\x86\vmon_bus.sys        %OUTPUT%\package\fre\x86\
copy bus\source\Release\x64\vmon_bus.sys        %OUTPUT%\package\fre\x64\

copy func\source\Debug\x86\vmon_func.inf        %OUTPUT%\package\chk\
copy func\source\Release\x86\vmon_func.inf      %OUTPUT%\package\fre\

copy func\source\Debug\x86\vmon_func.sys        %OUTPUT%\package\chk\x86\
copy func\source\Debug\x64\vmon_func.sys        %OUTPUT%\package\chk\x64\
copy func\source\Release\x86\vmon_func.sys      %OUTPUT%\package\fre\x86\
copy func\source\Release\x64\vmon_func.sys      %OUTPUT%\package\fre\x64\


copy notify\source\Debug\x86\notify.exe         %OUTPUT%\package\chk\x86\
copy notify\source\Debug\x64\notify.exe         %OUTPUT%\package\chk\x64\
copy notify\source\Release\x86\notify.exe       %OUTPUT%\package\fre\x86\
copy notify\source\Release\x64\notify.exe       %OUTPUT%\package\fre\x64\

REM
REM preparing PDB
REM
copy bus\source\Debug\x86\vmon_bus.pdb          %OUTPUT%\pdb\chk\x86\
copy bus\source\Debug\x64\vmon_bus.pdb          %OUTPUT%\pdb\chk\x64\
copy bus\source\Release\x86\vmon_bus.pdb        %OUTPUT%\pdb\fre\x86\
copy bus\source\Release\x64\vmon_bus.pdb        %OUTPUT%\pdb\fre\x64\

copy func\source\Debug\x86\vmon_func.pdb        %OUTPUT%\pdb\chk\x86\
copy func\source\Debug\x64\vmon_func.pdb        %OUTPUT%\pdb\chk\x64\
copy func\source\Release\x86\vmon_func.pdb      %OUTPUT%\pdb\fre\x86\
copy func\source\Release\x64\vmon_func.pdb      %OUTPUT%\pdb\fre\x64\

copy notify\source\Debug\x86\notify.pdb         %OUTPUT%\pdb\chk\x86\
copy notify\source\Debug\x64\notify.pdb         %OUTPUT%\pdb\chk\x64\
copy notify\source\Release\x86\notify.pdb       %OUTPUT%\pdb\fre\x86\
copy notify\source\Release\x64\notify.pdb       %OUTPUT%\pdb\fre\x64\

REM
REM run INF2CAT
REM
INF2CAT /uselocaltime /drv:.\%OUTPUT%\package\chk /os:Vista_X86,Vista_X64,7_X86,7_X64
INF2CAT /uselocaltime /drv:.\%OUTPUT%\package\fre /os:Vista_X86,Vista_X64,7_X86,7_X64

if "%TEST_SIGN%" == "" set TEST_SIGN=signtool.exe sign /v /s trustedpublisher /n ljbtest /t http://timestamp.verisign.com/scripts/timstamp.dll

%TEST_SIGN% %OUTPUT%\package\chk\vmon_bus.cat
%TEST_SIGN% %OUTPUT%\package\fre\vmon_bus.cat
%TEST_SIGN% %OUTPUT%\package\chk\x86\vmon_bus.sys
%TEST_SIGN% %OUTPUT%\package\chk\x64\vmon_bus.sys
%TEST_SIGN% %OUTPUT%\package\fre\x86\vmon_bus.sys
%TEST_SIGN% %OUTPUT%\package\fre\x64\vmon_bus.sys

%TEST_SIGN% %OUTPUT%\package\chk\vmon_func.cat
%TEST_SIGN% %OUTPUT%\package\fre\vmon_func.cat
%TEST_SIGN% %OUTPUT%\package\chk\x86\vmon_func.sys
%TEST_SIGN% %OUTPUT%\package\chk\x64\vmon_func.sys
%TEST_SIGN% %OUTPUT%\package\fre\x86\vmon_func.sys
%TEST_SIGN% %OUTPUT%\package\fre\x64\vmon_func.sys
