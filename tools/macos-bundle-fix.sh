#!/bin/bash
# Fix app bundle linking after macdeployqt:
# - Repoints Qt framework paths in all bundle binaries
# - Copies Homebrew libraries into Frameworks/ and fixes their paths (recursively)
# Usage: ./tools/macos-bundle-fix.sh [bundle-path]

set -euo pipefail

BUNDLE="${1:-install/quickevent.app}"
FRAMEWORKS="$BUNDLE/Contents/Frameworks"
mkdir -p "$FRAMEWORKS"

fix_binary() {
    local binary="$1"
    otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}' | while read -r dep; do
        case "$dep" in
            @*|/usr/lib/*|/System/*) ;;
            /opt/homebrew/*|/usr/local/*)
                local lib; lib=$(basename "$dep")
                if [[ ! -f "$FRAMEWORKS/$lib" ]]; then
                    echo "  Bundling $lib"
                    cp "$dep" "$FRAMEWORKS/$lib"
                    chmod 755 "$FRAMEWORKS/$lib"
                    install_name_tool -id "@executable_path/../Frameworks/$lib" \
                        "$FRAMEWORKS/$lib"
                    fix_binary "$FRAMEWORKS/$lib"
                fi
                install_name_tool -change "$dep" \
                    "@executable_path/../Frameworks/$lib" "$binary"
                ;;
            */Qt*.framework/*/Qt*)
                local fw; fw=$(basename "$dep")
                install_name_tool -change "$dep" \
                    "@executable_path/../Frameworks/$fw.framework/Versions/A/$fw" \
                    "$binary"
                ;;
        esac
    done
}

echo "Fixing bundle: $BUNDLE"
find "$BUNDLE/Contents/MacOS" -maxdepth 1 -type f | while read -r f; do fix_binary "$f"; done
find "$BUNDLE/Contents" -name "*.dylib" | while read -r f; do fix_binary "$f"; done
echo "Done"
