@echo off
setlocal

set "ROOT=%~dp0"
rem Der abschliessende Backslash darf bei CMake nicht direkt vor einem
rem schliessenden Anfuehrungszeichen stehen.
set "ROOT=%ROOT:~0,-1%"
set "BUILD=%ROOT%\build-vs2026"
set "VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
set "PYTHON_HOME=C:\Users\lumea\AppData\Local\Programs\Python\Python312"
set "WXROOT=%ROOT%\cache\wxWidgets"
set "WXLIB=%WXROOT%\lib\vc_dll"

if not exist "%VSROOT%\Common7\Tools\VsDevCmd.bat" (
  echo FEHLER: Visual Studio 2026 wurde nicht gefunden.
  exit /b 1
)

if not exist "%CMAKE%" (
  echo FEHLER: CMake wurde nicht gefunden.
  exit /b 1
)

if not exist "%WXROOT%\include\wx\wx.h" (
  echo FEHLER: wxWidgets wurde nicht eingerichtet.
  exit /b 1
)

set "PATH=%PYTHON_HOME%;%PYTHON_HOME%\Scripts;C:\Program Files\Git\cmd;C:\Program Files (x86)\Poedit\GettextTools\bin;C:\Program Files\7-Zip;%PATH%"

echo [1/4] Visual-Studio-2026-Buildumgebung laden ...
call "%VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

echo [2/4] Alten Buildordner entfernen ...
if exist "%BUILD%" rmdir /s /q "%BUILD%"

echo [3/4] Projekt konfigurieren ...
"%CMAKE%" -S "%ROOT%" -B "%BUILD%" -A Win32 -G "Visual Studio 18 2026" ^
  -DCMAKE_GENERATOR_PLATFORM=Win32 ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DwxWidgets_LIB_DIR="%WXLIB%" ^
  -DwxWidgets_ROOT_DIR="%WXROOT%" ^
  "-DOCPN_TARGET_TUPLE=msvc-wx32;10;x86_64"
if errorlevel 1 (
  echo FEHLER: CMake-Konfiguration fehlgeschlagen.
  exit /b %errorlevel%
)

echo [4/4] otcurrent_LTC_V.2.5 kompilieren und Plugin-Paket bauen ...
"%CMAKE%" --build "%BUILD%" --config RelWithDebInfo --target tarball
if errorlevel 1 (
  echo FEHLER: Build fehlgeschlagen.
  exit /b %errorlevel%
)

echo.
echo otcurrent_LTC_V.2.5 wurde erfolgreich gebaut.
echo Erzeugte Pakete:
dir /b "%BUILD%\*.tar.gz" 2>nul
dir /b "%BUILD%\*.zip" 2>nul
exit /b 0
