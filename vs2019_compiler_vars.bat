@echo on
:: Set env vars that tell distutils to use the compiler that we put on path
SET DISTUTILS_USE_SDK=1
:: This is probably not good. It is for the pre-UCRT msvccompiler.py *not* _msvccompiler.py
SET MSSdk=1

SET "VS_VERSION=17.0"
SET "VS_MAJOR=17"
SET "VS_YEAR=2022"

set "MSYS2_ARG_CONV_EXCL=/AI;/AL;/OUT;/out"
set "MSYS2_ENV_CONV_EXCL=CL"

:: For Python 3.5+, ensure that we link with the dynamic runtime.  See
:: http://stevedower.id.au/blog/building-for-python-3-5-part-two/ for more info
set "PY_VCRUNTIME_REDIST=%PREFIX%\bin\vcruntime140.dll"

:: set CC and CXX for cmake
set "CXX=cl.exe"
set "CC=cl.exe"

set "VSINSTALLDIR="
set "NEWER_VS_WITH_OLDER_VC=0"

:: Try to find actual vs2017 installations
for /f "usebackq tokens=*" %%i in (`vswhere.exe -nologo -products * -version ^[16.0^,17.0^) -property installationPath`) do (
  :: There is no trailing back-slash from the vswhere, and may make vcvars64.bat fail, so force add it
  set "VSINSTALLDIR=%%i\"
)
if not exist "%VSINSTALLDIR%" (
    :: VS2019 install but with vs2017 compiler stuff installed
	for /f "usebackq tokens=*" %%i in (`vswhere.exe -nologo -products * -requires Microsoft.VisualStudio.Component.VC.v143.x86.x64 -property installationPath`) do (
	:: There is no trailing back-slash from the vswhere, and may make vcvars64.bat fail, so force add it
	set "VSINSTALLDIR=%%i\"
	set "NEWER_VS_WITH_OLDER_VC=1"
	)
)
if not exist "%VSINSTALLDIR%" (
set "VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\"
)
if not exist "%VSINSTALLDIR%" (
set "VSINSTALLDIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community\"
)
if not exist "%VSINSTALLDIR%" (
set "VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\"
)
if not exist "%VSINSTALLDIR%" (
set "VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\"
)

IF NOT "%CONDA_BUILD%" == "" (
  set "INCLUDE=%LIBRARY_INC%;%INCLUDE%"
  set "LIB=%LIBRARY_LIB%;%LIB%"
  set "CMAKE_PREFIX_PATH=%LIBRARY_PREFIX%;%CMAKE_PREFIX_PATH%"
)


call :GetWin10SdkDir
:: dir /ON here is sorting the list of folders, such that we use the latest one that we have
for /F %%i in ('dir /ON /B "%WindowsSdkDir%\include\10.*"') DO (
  SET WindowsSDKVer=%%~i
)
if errorlevel 1 (
    echo "Didn't find any windows 10 SDK. I'm not sure if things will work, but let's try..."
) else (
    echo Windows SDK version found as: "%WindowsSDKVer%"
)

IF "win-64" == "win-64" (
    set "BITS=64"
    set "CMAKE_PLAT=x64"
) ELSE (
    set "BITS=32"
    set "CMAKE_PLAT=Win32"
)

:: set CMAKE_* variables
:: platform selection changed with VS 17 2022, but for compatibility we keep the older way
IF 2019 GEQ 2019  (
    set "CMAKE_GEN=Visual Studio 17 2022"
    set "USE_NEW_CMAKE_GEN_SYNTAX=1"
) ELSE (
    IF "win-64" == "win-64" (
        set "CMAKE_GEN=Visual Studio 17 2022 Win64"
	) else (
        set "CMAKE_GEN=Visual Studio 17 2022"
    )
    set "USE_NEW_CMAKE_GEN_SYNTAX=0"
)

echo "NEWER_VS_WITH_OLDER_VC=%NEWER_VS_WITH_OLDER_VC%"

IF "%NEWER_VS_WITH_OLDER_VC%" == "1" (
    set "CMAKE_GEN=Visual Studio 17 2022"
    set "USE_NEW_CMAKE_GEN_SYNTAX=1"
)

IF "%CMAKE_GENERATOR%" == "" SET "CMAKE_GENERATOR=%CMAKE_GEN%"
:: see https://cmake.org/cmake/help/latest/envvar/CMAKE_GENERATOR_PLATFORM.html
IF "%USE_NEW_CMAKE_GEN_SYNTAX%" == "1" (
	IF "%CMAKE_GENERATOR_PLATFORM%" == "" SET "CMAKE_GENERATOR_PLATFORM=%CMAKE_PLAT%"
	IF "%CMAKE_GENERATOR_TOOLSET%" == "" SET "CMAKE_GENERATOR_TOOLSET=v142"
)

pushd %VSINSTALLDIR%
CALL "VC\Auxiliary\Build\vcvars%BITS%.bat" -vcvars_ver=14.3 %WindowsSDKVer%
popd

:GetWin10SdkDir
call :GetWin10SdkDirHelper HKLM\SOFTWARE\Wow6432Node > nul 2>&1
if errorlevel 1 call :GetWin10SdkDirHelper HKCU\SOFTWARE\Wow6432Node > nul 2>&1
if errorlevel 1 call :GetWin10SdkDirHelper HKLM\SOFTWARE > nul 2>&1
if errorlevel 1 call :GetWin10SdkDirHelper HKCU\SOFTWARE > nul 2>&1
if errorlevel 1 exit /B 1
exit /B 0


:GetWin10SdkDirHelper
@REM `Get Windows 10 SDK installed folder`
for /F "tokens=1,2*" %%i in ('reg query "%1\Microsoft\Microsoft SDKs\Windows\v10.0" /v "InstallationFolder"') DO (
    if "%%i"=="InstallationFolder" (
        SET WindowsSdkDir=%%~k
    )
)
exit /B 0
