#!/bin/bash
# Package a self-contained, ad-hoc-signed Qt 6 app.
# Compatible with the Bash 3.2 shipped with macOS.
set -euo pipefail
cd "$(dirname "$0")/.."

[[ $(uname -s) == Darwin ]] || { echo 'macOS deployment requires macOS.' >&2; exit 1; }
qmake=${1:-qmake}
qt_bins=$("$qmake" -query QT_INSTALL_BINS)
macdeployqt="$qt_bins/macdeployqt"
[[ -x "$macdeployqt" ]] || { echo "Missing deployment tool: $macdeployqt" >&2; exit 1; }
for tool in ditto otool codesign file; do
    command -v "$tool" >/dev/null || { echo "Missing $tool; install Apple's Command Line Tools." >&2; exit 1; }
done

source_app=build/macos/TIATracker.app
[[ -x "$source_app/Contents/MacOS/TIATracker" ]] || { echo 'Build the application with make first.' >&2; exit 1; }
destination="$PWD/build/macos-deploy"
mkdir -p "$destination"
staging=$(mktemp -d "$destination/.deploy.XXXXXX")
trap 'rm -rf "$staging"' EXIT
app="$staging/TIATracker.app"

# Start fresh so removed libraries, plugins and examples do not survive a rebuild.
# Do not alter the development build or replace the last deployment on failure.
ditto "$source_app" "$app"
# Refresh the bundled defaults before signing; never ship stale build resources.
resources="$app/Contents/Resources/data"
rm -rf "$resources"
mkdir -p "$resources"
cp -R data/. "$resources/"
cp -R player instruments songs guides "$resources/"

# macdeployqt follows third-party dependencies too (SDL2, and SDL3 when using
# sdl2-compat), rewrites install names and supplies the Cocoa platform plugin.
# ARM64 requires valid signatures on modified code.
"$macdeployqt" "$app" -always-overwrite -codesign=-

# Some macdeployqt errors do not cause a nonzero exit status. Reject incomplete
# bundles and remaining external dependencies instead of reporting success.
[[ -f "$app/Contents/PlugIns/platforms/libqcocoa.dylib" ]] || {
    echo 'Deployment failed: Cocoa platform plugin is missing.' >&2; exit 1;
}
while IFS= read -r -d '' binary; do
    file -b "$binary" | grep -q 'Mach-O' || continue
    install_id=$(otool -D "$binary" | sed -n '2p')
    while IFS= read -r dependency; do
        # otool -L includes LC_ID_DYLIB, which is not a dependency to resolve.
        [[ "$dependency" == "$install_id" ]] && continue
        case "$dependency" in
            /System/Library/*|/usr/lib/*) continue ;;
            @rpath/*) resolved="$app/Contents/Frameworks/${dependency#@rpath/}" ;;
            @executable_path/*) resolved="$app/Contents/MacOS/${dependency#@executable_path/}" ;;
            @loader_path/*) resolved="$(dirname "$binary")/${dependency#@loader_path/}" ;;
            *) echo "Unbundled dependency: $dependency (in $binary)" >&2; exit 1 ;;
        esac
        [[ -e "$resolved" ]] || {
            echo "Missing bundled dependency: $dependency (in $binary)" >&2; exit 1;
        }
    done < <(otool -L "$binary" | sed -n '2,$s/^[[:space:]]*\(.*\) (compatibility version.*$/\1/p')
done < <(find "$app" -type f -print0)
codesign --verify --deep --strict "$app"

# The app can be moved on its own, including into /Applications.
rm -rf "$destination/TIATracker.app"
mv "$app" "$destination/TIATracker.app"
# Remove obsolete deployment outputs only after the new app is ready.
rm -f "$destination/TIATracker-macos.zip"
rm -rf "$destination/TIATracker"
echo 'macOS app ready: build/macos-deploy/TIATracker.app'
echo 'Move the app to Applications and launch it from Finder.'
echo 'Resources are copied to ~/Documents/TIATracker on startup; existing files are preserved.'
echo 'This is ad-hoc signed, not Developer ID signed or notarized.'