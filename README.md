# TIATracker #

A music tracker for making Atari VCS 2600 music on the PC, including a new sound routine for the VCS. Current version: 1.3

(c) 2016-2017 by Andre "Kylearan" Wichmann (andre.wichmann@gmx.de)

* Manual: [https://bitbucket.org/kylearan/tiatracker/raw/master/data/TIATracker_manual.pdf](https://bitbucket.org/kylearan/tiatracker/raw/master/data/TIATracker_manual.pdf)
* Windows binaries: [http://www.compoundeye.net/demoscene/TIATracker.zip](http://www.compoundeye.net/demoscene/TIATracker.zip)
* For Linux and OS X, use "wine" or compile the source (uses Qt and SDL)
* Source: [https://bitbucket.org/kylearan/tiatracker](https://bitbucket.org/kylearan/tiatracker)
* Seminar talk from Revision 2016 about VCS music in general and TIATracker in particular: [https://www.youtube.com/watch?v=PVujzQySZls](https://www.youtube.com/watch?v=PVujzQySZls)

## Features ##

VCS sound routine features:

* Up to 7 melodic and 15 percussion instruments
* ADSR envelopes for volume and frequency for melodic instruments
* "Overlay" percussion which will play the next melodic notes immediately
* Arbitrary and variable pattern lengths individual for each channel
* An option for different tempo values for odd and even rows ("Funkspeed")
* An option to have different tempo values per pattern instead of global tempo
* Highly optimized and configurable replayer routine
* Richly documented source code, including specifications for all data structures

Tracker features:

* Graphical representation for available notes per waveform and how off-tune they are
* Customizable pitch guides optimizing the number of in-tune notes
* Graphical editors for melodic and percussion instruments
* Integrated pattern editor and sequencer
* Timeline "mini map" displaying the pattern sequences
* Sound emulation from the Stella emulator for playback
* Export to dasm, k65 and .csv

For feedback, bug reports and feature requests, send a mail to andre.wichmann@gmx.de!

## Compiling from source

TIATracker requires Qt 5, SDL2 and a C++ compiler. Qt Creator is optional.

### Windows: MSYS2 UCRT64

You can build a native 64-bit Windows version of TIATracker using MSYS2.
**Install MSYS2 first** if you do not already have it:

1. Download the x86_64 installer from the [official MSYS2 website](https://www.msys2.org/).
2. Run the installer and follow its setup instructions. The default installation
   folder is `C:\msys64`.
3. Open **MSYS2 UCRT64** from the Windows Start menu.

Run the commands below in that UCRT64 terminal. First, install the build dependencies:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-SDL2
```

These include MSYS2's [Qt 5](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-qt5-base)
and [SDL2](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-SDL2)
packages for UCRT64. Use the UCRT64 packages together so the compiler and libraries match.

From the repository directory, build and launch with:

```sh
make
make run
```

The Makefile runs `qmake-qt5` (or `qmake` on older MSYS2 installations) and
`mingw32-make`, then copies the required data,
player sources and examples to `build/ucrt64/`. It uses the installed SDL2
package. You do not need a separate `make install` step.

The executable is `build/ucrt64/TIATracker.exe`. `make run` starts it from that
directory so it can find its data. Run from the UCRT64 terminal so the Qt and
SDL2 DLLs are available on `PATH`.

With Qt 5.6 or newer, the interface follows your display's DPI scaling, including
the track editor and piano keyboard. On Windows, adjust **Settings > System >
Display > Scale** to change the size of text and controls. Qt 5.14 or newer also
preserves fractional scale settings such as 125% and 150%.

To run from Windows Explorer without an MSYS2 terminal, create a Windows bundle:

```sh
make deploy
```

Double-click `build/windows/TIATracker.exe`. This folder includes the application
data, Qt plugins, SDL2, compiler runtime DLLs and their dependencies. Copy or zip
the **entire `build/windows/` folder**, rather than just the executable. No changes
to the Windows `PATH` are needed. Run `make deploy` again after rebuilding or
updating dependencies to refresh the bundle. It overwrites the bundled data and
examples, so save your own songs outside the build folder.

Other commands:

```sh
make JOBS=8    # Compile with eight parallel jobs (default: four)
make clean     # Remove compiled files; keep the copied data and examples
```

Qt's tools still generate the UI, resource and meta-object code from the
existing `TIATracker.pro` project. The top-level Makefile provides the command-line
entry point; invoke it with `make`, not `mingw32-make`.

### Qt Creator

Open `TIATracker.pro` in Qt Creator and add a `make install` build step, then
compile it. When using an MSYS2 UCRT64 kit, add `CONFIG+=system_sdl` to the qmake
arguments to use the installed SDL2 package.
