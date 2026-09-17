# Run with MSYS2's `make` from a UCRT64 terminal.
# qmake remains responsible for Qt's moc, uic and rcc build rules.
SHELL := /bin/sh
.DEFAULT_GOAL := all

# Older MSYS2 Qt 5 packages name the tool qmake instead of qmake-qt5.
QMAKE ?= $(shell command -v qmake-qt5 >/dev/null 2>&1 && echo qmake-qt5 || echo qmake)
BUILD_MAKE ?= mingw32-make
JOBS ?= 4

.PHONY: all check configure run deploy clean test-audio

all: configure
	$(BUILD_MAKE) -C build/ucrt64 -j$(JOBS)
	cp -R data/. build/ucrt64/
	cp -R player instruments songs guides build/ucrt64/

check:
	@test "$$MSYSTEM" = UCRT64 || { echo "Open an MSYS2 UCRT64 terminal first."; exit 1; }
	@for tool in $(QMAKE) $(BUILD_MAKE) g++ pkg-config; do \
		command -v "$$tool" >/dev/null || { echo "Missing $$tool; see README.md for dependencies."; exit 1; }; \
	done
	@$(QMAKE) -query QT_VERSION | grep -q '^5\.' || { echo "Qt 5 is required."; exit 1; }
	@pkg-config --exists sdl2 || { echo "SDL2 is missing; see README.md for dependencies."; exit 1; }

configure: check
	@mkdir -p build/ucrt64
	cd build/ucrt64 && $(QMAKE) ../../TIATracker.pro -spec win32-g++ \
		"CONFIG+=release" "CONFIG-=debug debug_and_release" "DESTDIR=."

test-audio: check
	@mkdir -p build/tests
	$(CXX) -std=c++11 -O2 -Wall -Wextra -I. $$(pkg-config --cflags sdl2) \
		tests/audio-scheduling.cpp emulation/SoundSDL2.cpp emulation/TIASnd.cpp \
		-o build/tests/audio-scheduling.exe $$(pkg-config --libs sdl2) -mconsole
	./build/tests/audio-scheduling.exe

# The application looks for its data relative to the working directory.
run: all
	cd build/ucrt64 && ./TIATracker.exe

deploy: all
	bash scripts/deploy-ucrt64.sh "$(QMAKE)"

clean:
	@if test -f build/ucrt64/Makefile; then \
		$(BUILD_MAKE) -C build/ucrt64 distclean; \
	fi
