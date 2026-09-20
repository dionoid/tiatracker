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

TIATracker requires Qt 5.15, SDL2 and a C++ compiler. Qt Creator is optional.

### Linux: Qt 5

Install a C++ compiler, Make, Qt 5.15 development libraries and tools (Core,
GUI, Widgets and qmake), SDL2 development libraries, and pkg-config using your
distribution's package manager.

Run `make` to build, `make run` to launch, or `make test-audio` to run the audio
scheduling tests. The Makefile uses `qmake-qt5` when available, otherwise
`qmake`; use `make QMAKE=/path/to/qt5/bin/qmake` to select a specific Qt 5
installation. Qt 6 is not supported.

The executable and copied application data are placed in `build/linux/`.
`make run` launches `build/linux/TIATracker` from that directory so it can
find its data. Qt and SDL2 must remain installed on the machine. `make clean`
and `make JOBS=8` are supported; Linux distribution packaging via `make deploy`
is not implemented.

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

The interface follows your display's DPI scaling, including
the track editor and piano keyboard. On Windows, adjust **Settings > System >
Display > Scale** to change the size of text and controls. It also
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
make test-audio # Check sample scheduling using SDL's silent dummy device
```

Playback register changes are scheduled at audio-sample boundaries: 882 samples
per PAL frame or 735 per NTSC frame at 44,100 Hz. The scheduler keeps two audio
buffers of lead (about 46 ms with the requested 1,024-sample buffers) to absorb
ordinary timer jitter. If a longer stall exhausts that lead, the current sound
continues until scheduling resumes; recovered frames retain their spacing.

Qt's tools still generate the UI, resource and meta-object code from the
existing `TIATracker.pro` project. The top-level Makefile provides the command-line
entry point; invoke it with `make`, not `mingw32-make`.

### macOS: Qt 5

Required libraries and build tools:

- **Qt 5.15** (`qt@5`): Qt Core, GUI and Widgets, plus the `qmake`, `moc`,
  `uic` and `rcc` build tools. Qt 6 is not supported by this build yet.
- **SDL2 development headers and libraries** (`sdl2`): audio output and the
  audio scheduling tests. See the compatibility-layer note below.
- **pkg-config** (`pkgconf`): lets qmake and the tests locate SDL2.
- **Apple Command Line Tools**: Clang, Make and the macOS SDK. A full Xcode
  installation is not required for this application build.

Homebrew installs the libraries' transitive dependencies automatically.
Qt Creator is optional.

Install Apple's Command Line Tools if they are not already installed:

```sh
xcode-select --install
```

With [Homebrew](https://brew.sh/) installed, install the dependencies:

```sh
brew install qt@5 sdl2 pkgconf
```

Qt 5 is deprecated in Homebrew but is retained here for the initial macOS port;
Qt 6 migration is a separate task. Homebrew currently schedules `qt@5` to be
disabled on May 19, 2027. Its `sdl2` formula now resolves to `sdl2-compat`, an
SDL2 API implementation backed by SDL3. An existing native SDL2 installation
also works, provided `pkg-config --exists sdl2` succeeds.

Use libraries built for the same architecture as the compiler (Apple Silicon
or Intel); do not mix native ARM64 dependencies with a Rosetta toolchain.

From the repository directory:

```sh
make
make run
make test-audio
```

The Makefile automatically selects Homebrew's keg-only Qt 5 `qmake` using
`brew --prefix qt@5`, uses Apple Clang and Make, and builds into `build/macos/`.
No global Qt PATH changes are needed. For a Qt 5 installation outside Homebrew,
pass its absolute qmake path, for example `make QMAKE=/path/to/qt5/bin/qmake`;
use the same override for `make run` and `make test-audio`.

**For development, launch using `make run`.** This starts the executable inside
`build/macos/TIATracker.app` with `build/macos/` as its working directory, where
the build copies the keyboard map, player templates and examples. The bundle
is a development build: it depends on installed Qt/SDL libraries. Use the ZIP
deployment below when copying to another machine.

`make JOBS=8` and `make clean` work on macOS too. Build commands refresh the
copied data and examples, so keep your own songs outside the build directory.

#### macOS ZIP distribution

To create a ZIP containing a Finder-launchable app and its external data files:

```sh
make deploy
```

The output is `build/macos-deploy/TIATracker-macos.zip`. Extract it and open
`TIATracker/TIATracker.app` from Finder. The archive contains:

```text
TIATracker/
  TIATracker.app
  keymap.cfg
  license.txt
  TIATracker_manual.pdf
  player/
  instruments/
  songs/
  guides/
```

An unpacked copy is also available in `build/macos-deploy/TIATracker/`.
**Keep the whole folder together**, rather than moving only the app to Applications.
Qt frameworks and plugins, SDL2 and their non-system library dependencies are
still inside the app, bundled using `macdeployqt` from the selected Qt 5 installation. When using
`sdl2-compat`, its SDL3 dependency is included too. Homebrew, Qt and SDL do not
need to be installed on the destination machine.

The keyboard map, manual, player export templates and examples are outside the
app, located relative to its containing folder rather than the working directory.
You can edit the external keymap or save songs, guides and instruments without
changing the app's code signature. Restart the app after changing the keymap.
Existing saved dialog locations are preserved. Deploying again replaces the
generated folder and ZIP, so keep your own work outside the build directory.

Deployment uses a fresh staging directory and verifies library dependencies and
code signatures and creates the ZIP before replacing the previous output. The development
bundle is left unchanged. The output targets the build machine's architecture,
not a universal Intel/Apple Silicon binary. The destination macOS version must
also be supported by the bundled Qt/SDL libraries; the app's compiler deployment
target alone does not guarantee compatibility with older macOS releases.

The app is **ad-hoc signed** for local use, not Developer ID signed or notarized.
Gatekeeper may block copies downloaded on another Mac. Public distribution
requires a separate Developer ID signing and notarization workflow; `make deploy`
does not perform those steps or create a disk image.

### Qt Creator

Open `TIATracker.pro` in Qt Creator and add a `make install` build step, then
compile it. SDL2 is resolved through `pkg-config`; ensure your kit uses the
matching SDL2 development package (UCRT64 for the MSYS2 setup above).
