#!/bin/bash
# Fast dev build-and-run loop for QuickEvent on macOS.
#
# Optimized for the edit -> build -> run cycle. The expensive Qt framework
# deployment (macdeployqt copies ~90 MB) runs only ONCE; afterwards each run
# only rebuilds, reinstalls the changed binaries, repoints/resigns them, and
# launches. Builds Debug by default and reuses ccache (see top-level CMakeLists).
#
# Usage:
#   ./dev-osx.sh [--release] [--no-run] [-- <app args>]
#   --release   Build Release instead of Debug
#   --no-run    Build + prepare the bundle but don't launch it
#   --          Everything after this is passed to the launched app
#
# For a clean, fully self-contained bundle (for distribution / DMG) use
# build-osx.sh instead; this script trades that for speed.
#
# Requirements: brew install qt ninja   (recommended: brew install ccache)
set -euo pipefail

WORKSPACE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$WORKSPACE/build"
INSTALL_DIR="$WORKSPACE/install"
APP="$INSTALL_DIR/quickevent.app"

BUILD_TYPE="Debug"
DO_RUN=1
APP_ARGS=()
while [[ $# -gt 0 ]]; do
	case "$1" in
		--release) BUILD_TYPE="Release" ;;
		--no-run)  DO_RUN=0 ;;
		--)        shift; APP_ARGS=("$@"); break ;;
		-h|--help) sed -n '2,18p' "$0"; exit 0 ;;
		*) echo "ERROR: unknown option: $1" >&2; exit 2 ;;
	esac
	shift
done

require() { command -v "$1" >/dev/null 2>&1 || { echo "ERROR: '$1' not found. $2" >&2; exit 1; }; }
require cmake "Run: brew install cmake"
require ninja "Run: brew install ninja"

QT_PREFIX="$(brew --prefix qt 2>/dev/null || true)"
if [[ -z "$QT_PREFIX" || ! -x "$QT_PREFIX/bin/macdeployqt" ]]; then
	echo "ERROR: Homebrew Qt not found. Run: brew install qt" >&2
	exit 1
fi
export PATH="$QT_PREFIX/bin:$PATH"

# --- Configure once (CMAKE_INSTALL_PREFIX must be set at configure time; see
#     the comment in build-osx.sh). Re-runs only when the cache is missing;
#     ninja itself re-invokes cmake if CMakeLists changes during the build. ---
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
	echo "==> Configuring ($BUILD_TYPE)"
	cmake -S "$WORKSPACE" -B "$BUILD_DIR" -G Ninja \
		-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
		-DUSE_QT6=ON \
		-DBUILD_TESTING=OFF \
		-DCOMMIT_SHA="$(git -C "$WORKSPACE" rev-parse --short HEAD)" \
		-DCMAKE_PREFIX_PATH="$QT_PREFIX" \
		-DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
fi

echo "==> Building"
cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu)"

echo "==> Installing"
cmake --install "$BUILD_DIR" >/dev/null

[[ -d "$APP" ]] || { echo "ERROR: bundle not produced: $APP" >&2; exit 1; }

# --- One-time: deploy Qt frameworks/plugins/QML into the bundle. Detected by
#     the presence of QtCore.framework; skipped on every later run. ---
if [[ ! -d "$APP/Contents/Frameworks/QtCore.framework" ]]; then
	echo "==> First run: deploying Qt frameworks (macdeployqt, one-time)"
	macdeployqt "$APP" \
		-qmldir="$WORKSPACE/quickevent/app/quickevent/plugins" \
		-always-overwrite
fi

# --- Always: the freshly reinstalled executable + our libs still reference Qt
#     by absolute Homebrew paths. Without this they load a SECOND copy of Qt
#     alongside the bundled one ("loading two sets of Qt binaries" -> abort).
#     install_name_tool invalidates code signatures, so re-sign afterwards. ---
echo "==> Fixing bundle linking + signing"
"$WORKSPACE/tools/macos-bundle-fix.sh" "$APP" >/dev/null
codesign --sign - --force --deep "$APP" >/dev/null 2>&1

echo "==> Ready: $APP"
if [[ "$DO_RUN" == 1 ]]; then
	echo "==> Launching"
	exec "$APP/Contents/MacOS/quickevent" ${APP_ARGS[@]+"${APP_ARGS[@]}"}
fi
