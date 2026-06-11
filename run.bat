@echo off
setlocal EnableDelayedExpansion

for %%I in ("%~dp0.") do set "REPO_ROOT=%%~fI"

if not defined QT_ROOT set "QT_ROOT=C:\Qt"
if not defined QT_VERSION set "QT_VERSION=6.11.1"
if not defined BUILD_DIR set "BUILD_DIR=%REPO_ROOT%\build"

set "APP_DIR=%BUILD_DIR%\quickevent\app\quickevent"
set "APP=%APP_DIR%\quickevent.exe"
set "TRANSLATIONS_DIR=%APP_DIR%\translations"
set "QT_TRANSLATIONS_DIR=%QT_ROOT%\%QT_VERSION%\mingw_64\translations"

if not exist "%APP%" (
	echo Missing executable: %APP%
	echo Run build.bat first.
	exit /b 1
)

if not exist "%QT_TRANSLATIONS_DIR%" (
	echo Missing Qt translations directory: %QT_TRANSLATIONS_DIR%
	exit /b 1
)

set "PATH=%BUILD_DIR%\3rdparty\necrolog;%BUILD_DIR%\libqf\libqfcore;%BUILD_DIR%\libqf\libqfgui;%BUILD_DIR%\libquickevent\libquickeventcore;%BUILD_DIR%\libquickevent\libquickeventgui;%BUILD_DIR%\libsiut;%QT_ROOT%\%QT_VERSION%\mingw_64\bin;%PATH%"

if not exist "%TRANSLATIONS_DIR%" mkdir "%TRANSLATIONS_DIR%"

for %%F in (
	"%BUILD_DIR%\libqf\libqfcore\*.qm"
	"%BUILD_DIR%\libqf\libqfgui\*.qm"
	"%BUILD_DIR%\libquickevent\libquickeventcore\*.qm"
	"%BUILD_DIR%\libquickevent\libquickeventgui\*.qm"
	"%BUILD_DIR%\libsiut\*.qm"
	"%APP_DIR%\quickevent-*.qm"
	"%QT_TRANSLATIONS_DIR%\qt_*.qm"
) do (
	if exist "%%~fF" copy /Y "%%~fF" "%TRANSLATIONS_DIR%\" >nul
)

set "APP_ARGS="
set "DRY_RUN="

if /I "%~1"=="cz" (
	set "APP_ARGS=--locale cs_CZ"
	shift
)
if /I "%~1"=="cs" (
	set "APP_ARGS=--locale cs_CZ"
	shift
)
if /I "%~1"=="czech" (
	set "APP_ARGS=--locale cs_CZ"
	shift
)

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--dry-run" (
	set "DRY_RUN=1"
) else (
	set "APP_ARGS=!APP_ARGS! "%~1""
)
shift
goto parse_args

:args_done

if defined DRY_RUN (
	echo APP=%APP%
	echo APP_DIR=%APP_DIR%
	echo TRANSLATIONS_DIR=%TRANSLATIONS_DIR%
	echo PATH=%PATH%
	echo ARGS=%APP_ARGS%
	exit /b 0
)

pushd "%APP_DIR%"
call "%APP%" %APP_ARGS%
set "APP_EXIT=%ERRORLEVEL%"
popd

exit /b %APP_EXIT%