@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT for %%v in (Community Professional Enterprise BuildTools) do if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%v\VC\Auxiliary\Build\vcvars64.bat" set "VSROOT=%ProgramFiles%\Microsoft Visual Studio\2022\%%v"
if not defined VSROOT (
  echo Visual Studio C++ build tools are required.
  exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
pushd "%~dp0.."
if not exist artifacts mkdir artifacts
if not exist .tools\unity\src\unity.c (
  git clone --depth 1 --branch v2.6.1 https://github.com/ThrowTheSwitch/Unity.git .tools\unity
  if errorlevel 1 exit /b 1
)
cl /nologo /EHsc /std:c++14 /I .tools\unity\src firmware\test\test_native\SimpleTests.cpp firmware\test\test_native\WiFiSetupStateMachineTests.cpp .tools\unity\src\unity.c /Fe:artifacts\native-tests.exe /Fo:artifacts\
if errorlevel 1 exit /b %errorlevel%
artifacts\native-tests.exe
exit /b %errorlevel%
