#!/bin/bash
# Build QuickEvent as a self-contained macOS .app bundle.
#
# Mirrors the steps of the macOS CI job (.github/workflows/macos.yml) so a local
# build matches what is released. Uses Homebrew Qt instead of install-qt-action.
#
# Usage:
#   ./build-osx.sh [options]
# Options:
#   --debug         Build with CMAKE_BUILD_TYPE=Debug (default: Release)
#   --postgresql    Bundle the PostgreSQL SQL driver (needs `brew install qt-postgresql`)
#   --dmg           Create a distributable .dmg (needs `brew install create-dmg`)
#   --no-deploy     Stop after install; skip macdeployqt/bundle-fix/codesign
#   --clean         Remove build/ and install/ before building
#   --run           Launch the built app when finished (alias: run)
#
# Requirements (install once):
#   brew install qt ninja
# Recommended (much faster clean rebuilds / branch switches, auto-detected by CMake):
#   brew install ccache
# Optional:
#   brew install qt-postgresql create-dmg
set -euo pipefail

# --- Resolve paths relative to this script, so it works from any cwd. ---
WORKSPACE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$WORKSPACE/build"
INSTALL_DIR="$WORKSPACE/install"

# --- Options ---
BUILD_TYPE="Release"
WITH_POSTGRESQL=0
WITH_DMG=0
DO_DEPLOY=1
DO_CLEAN=0
DO_RUN=0
for arg in "$@"; do
	case "$arg" in
		--debug)       BUILD_TYPE="Debug" ;;
		--postgresql)  WITH_POSTGRESQL=1 ;;
		--dmg)         WITH_DMG=1 ;;
		--no-deploy)   DO_DEPLOY=0 ;;
		--clean)       DO_CLEAN=1 ;;
		--run|run)     DO_RUN=1 ;;
		-h|--help)     sed -n '2,23p' "$0"; exit 0 ;;
		*) echo "ERROR: unknown option: $arg" >&2; exit 2 ;;
	esac
done

# --- Hard-fail early if a required tool is missing (no silent fallbacks). ---
require() {
	command -v "$1" >/dev/null 2>&1 || {
		echo "ERROR: '$1' not found. $2" >&2
		exit 1
	}
}
require brew   "Install Homebrew from https://brew.sh"
require cmake  "Run: brew install cmake"
require ninja  "Run: brew install ninja"

QT_PREFIX="$(brew --prefix qt 2>/dev/null || true)"
if [[ -z "$QT_PREFIX" || ! -x "$QT_PREFIX/bin/macdeployqt" ]]; then
	echo "ERROR: Homebrew Qt not found. Run: brew install qt" >&2
	exit 1
fi
export PATH="$QT_PREFIX/bin:$PATH"

echo "==> Workspace:   $WORKSPACE"
echo "==> Qt:          $QT_PREFIX ($("$QT_PREFIX/bin/qmake" -query QT_VERSION))"
echo "==> Build type:  $BUILD_TYPE"

if [[ "$DO_CLEAN" == 1 ]]; then
	echo "==> Cleaning build/ and install/"
	rm -rf "$BUILD_DIR" "$INSTALL_DIR"
fi

# --- Submodules (necrolog) must be present. ---
echo "==> Updating git submodules"
git -C "$WORKSPACE" submodule update --init --recursive

COMMIT_SHA="$(git -C "$WORKSPACE" rev-parse --short HEAD)"

# --- Configure ---
# CMAKE_INSTALL_PREFIX MUST be set here, not just at `cmake --install` time:
# the macOS bundling step in quickevent/app/quickevent/CMakeLists.txt bakes
# ${CMAKE_INSTALL_PREFIX} into an install(CODE ...) block at configure time and
# globs "<prefix>/lib/*.dylib". With the default /usr/local prefix it would both
# write to the wrong place and sweep up unrelated dylibs from /usr/local/lib.
echo "==> Configuring"
cmake \
	-S "$WORKSPACE" \
	-B "$BUILD_DIR" \
	-G Ninja \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DQF_BUILD_QML_PLUGINS=ON \
	-DBUILD_TESTING=OFF \
	-DUSE_QT6=ON \
	-DCOMMIT_SHA="$COMMIT_SHA" \
	-DCMAKE_PREFIX_PATH="$QT_PREFIX" \
	-DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

# --- Build ---
echo "==> Building"
cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu)"

# --- Install (assembles install/quickevent.app) ---
echo "==> Installing into $INSTALL_DIR"
rm -rf "$INSTALL_DIR"
cmake --install "$BUILD_DIR"

APP="$INSTALL_DIR/quickevent.app"
[[ -d "$APP" ]] || { echo "ERROR: bundle not produced: $APP" >&2; exit 1; }

if [[ "$DO_DEPLOY" == 0 ]]; then
	echo "==> Skipping deploy (--no-deploy). Bundle is NOT self-contained yet:"
	echo "    $APP"
	if [[ "$DO_RUN" == 1 ]]; then
		echo "WARNING: --run ignored with --no-deploy; the app would fail to find Qt frameworks." >&2
	fi
	exit 0
fi

# --- Optional: PostgreSQL SQL driver (SQLite needs nothing extra). ---
if [[ "$WITH_POSTGRESQL" == 1 ]]; then
	PG_PREFIX="$(brew --prefix qt-postgresql 2>/dev/null || true)"
	PG_DRIVER="$PG_PREFIX/share/qt/plugins/sqldrivers/libqsqlpsql.dylib"
	[[ -f "$PG_DRIVER" ]] || {
		echo "ERROR: PostgreSQL driver not found. Run: brew install qt-postgresql" >&2
		exit 1
	}
	echo "==> Bundling PostgreSQL driver"
	mkdir -p "$APP/Contents/PlugIns/sqldrivers"
	cp "$PG_DRIVER" "$APP/Contents/PlugIns/sqldrivers/"
fi

# --- Bundle Qt frameworks + QML modules. ---
echo "==> Running macdeployqt"
macdeployqt "$APP" \
	-qmldir="$WORKSPACE/quickevent/app/quickevent/plugins" \
	-always-overwrite

# --- Repoint leftover absolute paths into the bundle (Qt + Homebrew libs). ---
echo "==> Fixing bundle linking"
chmod +x "$WORKSPACE/tools/macos-bundle-fix.sh"
"$WORKSPACE/tools/macos-bundle-fix.sh" "$APP"

# --- Ad-hoc code sign (must be last; install_name_tool invalidates signatures). ---
echo "==> Code signing (ad-hoc)"
codesign --sign - --force --deep "$APP"

echo ""
echo "==> Done: $APP"

# --- Optional: DMG ---
if [[ "$WITH_DMG" == 1 ]]; then
	require create-dmg "Run: brew install create-dmg"
	VERSION="$(grep APP_VERSION "$WORKSPACE/quickevent/app/quickevent/src/appversion.h" | cut -d'"' -f2)"
	DMG="$WORKSPACE/quickevent-$VERSION.dmg"
	echo "==> Creating DMG: $DMG"
	rm -f "$DMG"
	create-dmg \
		--volname "QuickEvent $VERSION" \
		--window-pos 200 120 \
		--window-size 600 400 \
		--icon-size 100 \
		--icon "quickevent.app" 175 190 \
		--hide-extension "quickevent.app" \
		--app-drop-link 425 185 \
		"$DMG" \
		"$APP"
	echo "==> DMG ready: $DMG"
fi

# --- Optional: launch the freshly built app. ---
if [[ "$DO_RUN" == 1 ]]; then
	echo "==> Launching $APP"
	exec "$APP/Contents/MacOS/quickevent"
fi
