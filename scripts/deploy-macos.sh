#!/bin/bash
# Package an ad-hoc-signed Qt 5 app and its sibling data files in a ZIP.
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
package="$staging/TIATracker"
mkdir -p "$package"
app="$package/TIATracker.app"

# Start fresh so removed libraries, plugins and examples do not survive a rebuild.
# Do not alter the development build or replace the last deployment on failure.
ditto "$source_app" "$app"
cp -R data/. "$package/"
cp -R player instruments songs guides "$package/"

# macdeployqt follows third-party dependencies too (SDL2, and SDL3 when using
# sdl2-compat), rewrites install names and supplies the Cocoa platform plugin.
# ARM64 requires valid signatures on modified code.
# Qt 5 scans WebP after relocating it, so its @loader_path/../lib rpath no
# longer points at Homebrew. -libpath does not apply to @rpath resolution.
# Stage this sibling first so the bundle's own Frameworks rpath resolves it.
if command -v brew >/dev/null 2>&1; then
    sharpyuv="$(brew --prefix)/lib/libsharpyuv.0.dylib"
    qt_plugins=$("$qmake" -query QT_INSTALL_PLUGINS)
    if [[ -f "$qt_plugins/imageformats/libqwebp.dylib" && -f "$sharpyuv" ]]; then
        mkdir -p "$app/Contents/Frameworks"
        cp -L "$sharpyuv" "$app/Contents/Frameworks/"
    fi
fi
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

# Preserve framework symlinks, executable permissions and macOS metadata.
# Include a parent folder so extracting the ZIP keeps the app and data together.
archive="$staging/TIATracker-macos.zip"
ditto -c -k --sequesterRsrc --keepParent "$package" "$archive"

rm -rf "$destination/TIATracker"
mv "$package" "$destination/TIATracker"
mv -f "$archive" "$destination/TIATracker-macos.zip"
# Remove the obsolete app-only output from the previous deployment layout.
rm -rf "$destination/TIATracker.app"
echo 'macOS ZIP ready: build/macos-deploy/TIATracker-macos.zip'
echo 'Unpacked folder: build/macos-deploy/TIATracker/'
echo 'Keep the app and its sibling data together; launch the app from Finder.'
echo 'This is ad-hoc signed, not Developer ID signed or notarized.'