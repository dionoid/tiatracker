#!/usr/bin/env bash
# Bundle the UCRT64 executable, Qt plugins and their recursive DLL dependencies.
set -euo pipefail
cd "$(dirname "$0")/.."

[[ ${MSYSTEM:-} == UCRT64 ]] || { echo 'Use an MSYS2 UCRT64 terminal.' >&2; exit 1; }
qmake=${1:-qmake}
runtime_dir=/ucrt64/bin
plugin_dir=$(cygpath -u "$("$qmake" -query QT_INSTALL_PLUGINS)")
system_dir=$(cygpath -u "$SYSTEMROOT")/System32
destination=build/windows
mkdir -p "$destination/platforms"
cp build/ucrt64/TIATracker.exe "$destination/"
cp -R data/. "$destination/"
cp -R player instruments songs guides "$destination/"
cp "$plugin_dir/platforms/qwindows.dll" "$destination/platforms/"

shopt -s nullglob
queue=("$destination/TIATracker.exe" "$destination/platforms/qwindows.dll")
for group in imageformats styles; do
    mkdir -p "$destination/$group"
    for plugin in "$plugin_dir/$group/"*.dll; do
        cp "$plugin" "$destination/$group/"
        queue+=("$destination/$group/${plugin##*/}")
    done
done

# Restrict Qt's plugin lookup to the bundled directory.
printf '[Paths]\nPrefix=.\nPlugins=.\n' > "$destination/qt.conf"

# DLLs can import further DLLs (e.g. Qt -> HarfBuzz -> FreeType).
# Index names without case because Windows DLL imports are case-insensitive.
declare -A runtime_dlls copied
for dll in "$runtime_dir/"*.dll; do
    name=${dll##*/}
    runtime_dlls[${name,,}]=$dll
done
for ((index=0; index<${#queue[@]}; ++index)); do
    imports=$("$runtime_dir/objdump.exe" -p "${queue[index]}")
    while read -r name; do
        key=${name,,}
        [[ -n ${copied[$key]:-} ]] && continue
        if [[ -n ${runtime_dlls[$key]:-} ]]; then
            dll=${runtime_dlls[$key]}
            cp "$dll" "$destination/"
            copied[$key]=1
            queue+=("$destination/${dll##*/}")
        elif [[ $key == api-ms-* || $key == ext-ms-* || -f "$system_dir/$name" ]]; then
            continue # Provided by Windows; never bundle system DLLs.
        else
            echo "Unresolved DLL: $name (required by ${queue[index]})" >&2
            exit 1
        fi
    done <<< "$(awk '/DLL Name:/ { sub(/\r$/, "", $3); print $3 }' <<< "$imports")"
done

echo "Windows bundle ready: $destination/TIATracker.exe"
echo 'Keep the entire windows folder together when copying or zipping it.'
