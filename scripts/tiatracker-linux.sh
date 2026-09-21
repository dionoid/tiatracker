#!/bin/sh
# The application seeds and uses ~/Documents/TIATracker on startup.
set -eu
exec /usr/lib/tiatracker/TIATracker "$@"
