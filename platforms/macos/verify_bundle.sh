#!/bin/bash
#
# Check that an app bundle carries everything it needs.
#
# install_name_tool exits 0 when the path it was asked to change is not
# present, so a rewrite that matches nothing looks exactly like one that
# worked. That is how a bundle once shipped with all three executables still
# pointing at /opt/homebrew/opt/qt@5 - every build was green and the app only
# failed on a machine without Homebrew Qt installed.
#
# So: walk every Mach-O in the bundle and fail if any of them still reference
# a library outside it. Only system paths are acceptable.
#
# Usage: verify_bundle.sh [/path/to/Foo.app]

set -u

BUNDLE="${1:-$HOME/QLC+.app}"

if [ ! -d "$BUNDLE" ]; then
    echo "verify_bundle: no bundle at $BUNDLE"
    exit 1
fi

# Anything resolved at runtime from inside the bundle, or provided by macOS.
is_ok() {
    case "$1" in
        @executable_path/*|@loader_path/*|@rpath/*) return 0 ;;
        /usr/lib/*|/System/*)                       return 0 ;;
        *)                                          return 1 ;;
    esac
}

bad=0
checked=0

while IFS= read -r f; do
    # Mach-O only; skip resources, plists, icons and so on
    file -b "$f" 2>/dev/null | grep -q "Mach-O" || continue
    checked=$((checked + 1))

    # For a library, otool -L reports the library's own install name as its
    # first entry. That is not a dependency, and for a plugin loaded by path it
    # does not matter what it says, so leave it out of the comparison.
    self_id=$(otool -D "$f" 2>/dev/null | tail -n +2 | head -1)

    while IFS= read -r dep; do
        [ -n "$dep" ] || continue
        [ -n "$self_id" ] && [ "$dep" = "$self_id" ] && continue
        if ! is_ok "$dep"; then
            if [ "$bad" -eq 0 ]; then
                echo "verify_bundle: libraries referenced from outside the bundle"
                echo
            fi
            printf '  %s\n      %s\n' "${f#$BUNDLE/}" "$dep"
            bad=$((bad + 1))
        fi
    done < <(otool -L "$f" 2>/dev/null | tail -n +2 | awk '{print $1}')
done < <(find "$BUNDLE" -type f)

echo
if [ "$bad" -gt 0 ]; then
    echo "verify_bundle: FAILED - $bad external reference(s) across $checked Mach-O files"
    echo "verify_bundle: the bundle will not run on a machine without those libraries"
    exit 1
fi

echo "verify_bundle: ok - $checked Mach-O files, no external references"
