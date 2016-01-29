@echo off
SETLOCAL
IF "%WindowsSDKDir%" == "" (
    echo Invoking VS2015 build environment.
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
)

nmake2msbuild.exe proxy-wddm\kmd\sources -NoPackageProject -NoSolution

msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Debug;Platform=Win32;TargetOsVersion=Win7
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Release;Platform=Win32;TargetOsVersion=Win7
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Debug;Platform=x64;TargetOsVersion=Win7
msbuild proxy-wddm\kmd\ljb_proxykmd.vcxproj /p:Configuration=Release;Platform=x64;TargetOsVersion=Win7

ENDLOCAL