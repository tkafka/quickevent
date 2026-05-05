#!/bin/bash
# Fix app bundle linking after macdeployqt:
# - Repoints Qt framework paths in all bundle binaries
# - Copies Homebrew libraries into Frameworks/ and fixes their paths (recursively)
# Usage: ./tools/macos-bundle-fix.sh [bundle-path]

set -euo pipefail

BUNDLE="${1:-install/quickevent.app}"

if [[ ! -d "$BUNDLE" ]]; then
    echo "ERROR: bundle not found: $BUNDLE"
    exit 1
fi

FRAMEWORKS="$BUNDLE/Contents/Frameworks"
mkdir -p "$FRAMEWORKS"

fix_binary() {
    local binary="$1"
    local binname; binname=$(basename "$binary")
    otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}' | while read -r dep; do
        # skip own install name (always appears as first otool -L entry for dylibs)
        [[ "$(basename "$dep")" == "$binname" ]] && continue
        case "$dep" in
            @*|/usr/lib/*|/System/*) ;;
            */Qt*.framework/*/Qt*)
                local fw; fw=$(basename "$dep")
                install_name_tool -change "$dep" \
                    "@executable_path/../Frameworks/$fw.framework/Versions/A/$fw" \
                    "$binary"
                ;;
            /*)
                local lib; lib=$(basename "$dep")
                local src="$dep"
                if [[ ! -f "$src" ]]; then
                    src=$(find /opt/homebrew /usr/local \( -type f -o -type l \) -name "$lib" 2>/dev/null | head -1)
                    if [[ -z "$src" ]]; then
                        echo "  WARNING: $lib not found, skipping"
                        continue
                    fi
                fi
                if [[ ! -f "$FRAMEWORKS/$lib" ]]; then
                    echo "  Bundling $lib"
                    cp "$src" "$FRAMEWORKS/$lib"
                    chmod 755 "$FRAMEWORKS/$lib"
                    install_name_tool -id "@executable_path/../Frameworks/$lib" \
                        "$FRAMEWORKS/$lib"
                    fix_binary "$FRAMEWORKS/$lib"
                fi
                install_name_tool -change "$dep" \
                    "@executable_path/../Frameworks/$lib" "$binary"
                ;;
        esac
    done
}

echo "Fixing bundle: $BUNDLE"
find "$BUNDLE/Contents/MacOS" -maxdepth 1 -type f | while read -r f; do fix_binary "$f"; done
find "$BUNDLE/Contents" -name "*.dylib" | while read -r f; do fix_binary "$f"; done
echo "Done"
