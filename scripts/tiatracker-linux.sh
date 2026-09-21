#!/bin/sh
# Keep editable data per user, while package upgrades refresh player templates.
set -eu
shared=/usr/share/tiatracker
userdata=${XDG_DATA_HOME:-"$HOME/.local/share"}/tiatracker
mkdir -p "$userdata"
for item in keymap.cfg instruments songs guides; do
    cp -Rn "$shared/$item" "$userdata/"
done
for item in player TIATracker_manual.pdf license.txt; do
    if [ ! -e "$userdata/$item" ] && [ ! -L "$userdata/$item" ]; then
        ln -s "$shared/$item" "$userdata/$item"
    fi
done
cd "$userdata"
TIATRACKER_DATA_DIR=$PWD
export TIATRACKER_DATA_DIR
exec /usr/lib/tiatracker/TIATracker "$@"
