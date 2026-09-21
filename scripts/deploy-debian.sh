#!/usr/bin/env bash
# Build a native Debian/Mint package using the host's library dependency metadata.
set -euo pipefail
cd "$(dirname "$0")/.."

for tool in dpkg dpkg-deb dpkg-shlibdeps strip; do
    command -v "$tool" >/dev/null || {
        echo "Missing $tool. Install packaging tools: sudo apt install dpkg-dev binutils" >&2
        exit 1
    }
done
version=${1:-1.3.1-1}
[[ $version =~ ^[0-9][A-Za-z0-9.+~-]*$ ]] || {
    echo "Invalid package version: $version" >&2
    exit 1
}
dpkg --validate-version "$version"
architecture=$(dpkg --print-architecture)
destination="$PWD/build/debian"
mkdir -p "$destination"
stage=$(mktemp -d "$destination/.stage.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT
package="$stage/package"
mkdir -p "$package/DEBIAN" "$package/usr/bin" "$package/usr/lib/tiatracker" \
    "$package/usr/share/tiatracker" "$package/usr/share/applications" \
    "$package/usr/share/icons/hicolor/256x256/apps" "$package/usr/share/doc/tiatracker"
install -m 755 build/linux/TIATracker "$package/usr/lib/tiatracker/TIATracker"
strip --strip-unneeded "$package/usr/lib/tiatracker/TIATracker"
install -m 755 scripts/tiatracker-linux.sh "$package/usr/bin/tiatracker"
cp -R data/. "$package/usr/share/tiatracker/"
cp -R player instruments songs guides "$package/usr/share/tiatracker/"
install -m 644 scripts/tiatracker.desktop "$package/usr/share/applications/tiatracker.desktop"
install -m 644 graphics/tt_icon.png "$package/usr/share/icons/hicolor/256x256/apps/tiatracker.png"
install -m 644 license.txt "$package/usr/share/doc/tiatracker/copyright"

# dpkg-shlibdeps needs a source control file even when emitting to stdout.
mkdir -p "$stage/debian"
cat > "$stage/debian/control" <<'EOF'
Source: tiatracker
Section: sound
Priority: optional
Maintainer: TIATracker contributors <tiatracker@localhost>

Package: tiatracker
Architecture: any
Description: Music tracker for the Atari VCS 2600
EOF
dependencies=$(cd "$stage" && dpkg-shlibdeps -O -e"$package/usr/lib/tiatracker/TIATracker")
dependencies=${dependencies#shlibs:Depends=}
[[ -n $dependencies && $dependencies != *$'\n'* ]] || {
    echo 'Unable to determine runtime dependencies.' >&2
    exit 1
}
cat > "$package/DEBIAN/control" <<EOF
Package: tiatracker
Version: $version
Architecture: $architecture
Section: sound
Priority: optional
Maintainer: TIATracker contributors <tiatracker@localhost>
Depends: $dependencies, qt6-qpa-plugins
Recommends: qt6-wayland
Installed-Size: $(du -sk "$package/usr" | cut -f1)
Description: Music tracker for the Atari VCS 2600
 Compose Atari VCS music with instrument editors, sound emulation,
 player export templates, examples and a manual.
EOF
# Normalize copied source permissions; dpkg supplies root ownership without sudo.
find "$package" -type d -exec chmod 755 {} +
find "$package/usr/share" -type f -exec chmod 644 {} +
chmod 644 "$package/DEBIAN/control"
archive="tiatracker_${version}_${architecture}.deb"
dpkg-deb --root-owner-group --build "$package" "$stage/$archive"
mv -f "$stage/$archive" "$destination/$archive"
echo "Debian package ready: build/debian/$archive"
echo "Install with: sudo apt install ./build/debian/$archive"
