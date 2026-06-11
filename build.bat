@echo off
setlocal

for %%I in ("%~dp0.") do set "REPO_ROOT=%%~fI"

if not defined QT_ROOT set "QT_ROOT=C:\Qt"
if not defined QT_VERSION set "QT_VERSION=6.11.1"
if not defined MINGW_VERSION set "MINGW_VERSION=mingw1310_64"
if not defined BUILD_DIR set "BUILD_DIR=%REPO_ROOT%\build"
if not defined BUILD_JOBS set "BUILD_JOBS=14"

set "MINGW_BIN=%QT_ROOT%\Tools\%MINGW_VERSION%\bin"
set "CMAKE_BIN=%QT_ROOT%\Tools\CMake_64\bin"
set "QT_PREFIX=%QT_ROOT%\%QT_VERSION%\mingw_64"

if not exist "%REPO_ROOT%\3rdparty\necrolog\CMakeLists.txt" (
	echo Missing 3rdparty\necrolog submodule.
	echo Run: git submodule update --init --recursive
	exit /b 1
)

if not exist "%MINGW_BIN%\gcc.exe" (
	echo Missing compiler: %MINGW_BIN%\gcc.exe
	exit /b 1
)

if not exist "%MINGW_BIN%\g++.exe" (
	echo Missing compiler: %MINGW_BIN%\g++.exe
	exit /b 1
)

if not exist "%CMAKE_BIN%\cmake.exe" (
	echo Missing CMake: %CMAKE_BIN%\cmake.exe
	exit /b 1
)

set "PATH=%MINGW_BIN%;%CMAKE_BIN%;%PATH%"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
	echo Configuring QuickEvent in %BUILD_DIR%...
	"%CMAKE_BIN%\cmake.exe" -G "MinGW Makefiles" -S "%REPO_ROOT%" -B "%BUILD_DIR%" ^
		-DCMAKE_BUILD_TYPE=Release ^
		-DCMAKE_C_COMPILER=%MINGW_BIN%\gcc.exe ^
		-DCMAKE_CXX_COMPILER=%MINGW_BIN%\g++.exe ^
		-DCMAKE_PREFIX_PATH=%QT_PREFIX% ^
		-DBUILD_SHARED_LIBS=ON ^
		-DUSE_QT6=ON ^
		-DBUILD_TESTING=OFF
	if errorlevel 1 exit /b %errorlevel%
)

echo Building quickevent...
"%CMAKE_BIN%\cmake.exe" --build "%BUILD_DIR%" --target quickevent -j %BUILD_JOBS%
exit /b %errorlevel%