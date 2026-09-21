# qmake recursively expands directories itself. Passing recursive glob results
# includes both directories and their children, embedding nested files twice.
defaults.files = $$PWD/data
defaults.prefix = /defaults
defaults.base = $$PWD/data
examples.files = $$PWD/player $$PWD/instruments $$PWD/songs $$PWD/guides
examples.prefix = /defaults
examples.base = $$PWD
RESOURCES += defaults examples
