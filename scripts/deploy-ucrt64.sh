#!/usr/bin/env bash
# Embed the application, Qt plugins and recursive DLL dependencies in one EXE.
set -euo pipefail
cd "$(dirname "$0")/.."

[[ ${MSYSTEM:-} == UCRT64 ]] || { echo 'Use an MSYS2 UCRT64 terminal.' >&2; exit 1; }
qmake=${1:-qmake}
runtime_dir=/ucrt64/bin
plugin_dir=$(cygpath -u "$("$qmake" -query QT_INSTALL_PLUGINS)")
system_dir=$(cygpath -u "$SYSTEMROOT")/System32
mkdir -p build/windows
destination=$(mktemp -d build/windows/runtime.XXXXXX)
trap 'rm -rf -- "$destination"' EXIT
mkdir -p "$destination/platforms"
cp build/ucrt64/TIATracker.exe "$destination/"
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

# Generate a Win32 resource table and matching paths for the native bootstrap.
resource_script="$destination/payload.rc"
header="$destination/payload.h"
printf '1 ICON "%s"\n' "$(cygpath -m "$PWD/graphics/tt_icon.ico")" > "$resource_script"
printf 'static const struct { unsigned short id; const wchar_t *path; } payload[] = {\n' > "$header"
id=10
while IFS= read -r -d '' file; do
    relative=${file#"$destination/"}
    printf '%s RCDATA "%s"\n' "$id" "$(cygpath -m "$PWD/$file")" >> "$resource_script"
    printf '    {%s, L"%s"},\n' "$id" "$relative" >> "$header"
    id=$((id + 1))
done < <(find "$destination" -type f ! -name 'payload.rc' ! -name 'payload.h' -print0 | sort -z)
printf '};\n' >> "$header"
"$runtime_dir/windres.exe" "$resource_script" -O coff -o "$destination/payload.o"
"$runtime_dir/g++.exe" -std=c++17 -Os -static -municode -mwindows \
    -I"$destination" scripts/windows-launcher.cpp "$destination/payload.o" \
    -lole32 -o "$destination/standalone.exe"
mv "$destination/standalone.exe" build/windows/TIATracker.exe
echo 'Windows standalone executable ready: build/windows/TIATracker.exe'
