# Run with Linux/macOS `make` or MSYS2's `make` from a UCRT64 terminal.
# qmake remains responsible for Qt's moc, uic and rcc build rules.
SHELL := /bin/sh
.DEFAULT_GOAL := all

HOST_OS := $(shell uname -s)
WINDOWS_HOST := $(filter MINGW% MSYS% CYGWIN%,$(HOST_OS))
DEPENDENCY_HINT := See README.md for dependencies.
ifeq ($(HOST_OS),Darwin)
# Select Homebrew's Qt 6 base tools.
QMAKE ?= $(shell if command -v brew >/dev/null 2>&1; then printf '%s/bin/qmake' "$$(brew --prefix qtbase)"; else command -v qmake6 >/dev/null 2>&1 && echo qmake6 || echo qmake; fi)
BUILD_MAKE ?= $(MAKE)
ifeq ($(origin CXX),default)
CXX := clang++
endif
BUILD_DIR := build/macos
QMAKE_SPEC :=
APP := TIATracker.app/Contents/MacOS/TIATracker
EXE_SUFFIX :=
TEST_LDFLAGS :=
else ifneq ($(WINDOWS_HOST),)
# MSYS2 installs Qt 6 tools with a versioned suffix.
QMAKE ?= $(shell command -v qmake6 >/dev/null 2>&1 && echo qmake6 || echo qmake)
BUILD_MAKE ?= mingw32-make
BUILD_DIR := build/ucrt64
QMAKE_SPEC := -spec win32-g++
APP := TIATracker.exe
EXE_SUFFIX := .exe
TEST_LDFLAGS := -mconsole
else
# Use the native Linux toolchain and let qmake select its default platform spec.
DEPENDENCY_HINT := On Debian/Mint: sudo apt install build-essential pkg-config qt6-base-dev qt6-base-dev-tools qmake6 libsdl2-dev
QMAKE ?= $(shell command -v qmake6 >/dev/null 2>&1 && echo qmake6 || echo qmake)
BUILD_MAKE ?= $(MAKE)
BUILD_DIR := build/linux
QMAKE_SPEC :=
APP := TIATracker
EXE_SUFFIX :=
TEST_LDFLAGS :=
endif
JOBS ?= 4
DEB_VERSION ?= 1.3.1-1

.PHONY: all check configure run deploy clean test-audio

all: configure
	$(BUILD_MAKE) -C $(BUILD_DIR) -j$(JOBS)
	cp -R data/. $(BUILD_DIR)/
	cp -R player instruments songs guides $(BUILD_DIR)/

check:
ifeq ($(HOST_OS),Darwin)
	@xcode-select -p >/dev/null 2>&1 || { echo "Install Apple's Command Line Tools; see README.md."; exit 1; }
else ifneq ($(WINDOWS_HOST),)
	@test "$$MSYSTEM" = UCRT64 || { echo "Open an MSYS2 UCRT64 terminal first."; exit 1; }
endif
	@for tool in "$(QMAKE)" "$(BUILD_MAKE)" "$(CXX)" pkg-config; do \
		command -v "$$tool" >/dev/null || { echo "Missing $$tool. $(DEPENDENCY_HINT)"; exit 1; }; \
	done
	@"$(QMAKE)" -query QT_VERSION | grep -q '^6\.' || { echo "Qt 6 is required. $(DEPENDENCY_HINT)"; exit 1; }
	@pkg-config --exists sdl2 || { echo "SDL2 development files are missing. $(DEPENDENCY_HINT)"; exit 1; }

configure: check
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && "$(QMAKE)" ../../TIATracker.pro $(QMAKE_SPEC) \
		"CONFIG+=release" "CONFIG-=debug debug_and_release" "DESTDIR=."

test-audio: check
	@mkdir -p build/tests
	$(CXX) -std=c++17 -O2 -Wall -Wextra -I. $$(pkg-config --cflags sdl2) \
		tests/audio-scheduling.cpp emulation/SoundSDL2.cpp emulation/TIASnd.cpp \
		-o build/tests/audio-scheduling$(EXE_SUFFIX) $$(pkg-config --libs sdl2) $(TEST_LDFLAGS)
	./build/tests/audio-scheduling$(EXE_SUFFIX)

# The application looks for its data relative to the working directory.
run: all
	cd $(BUILD_DIR) && ./$(APP)

ifeq ($(HOST_OS),Darwin)
deploy: all
	bash scripts/deploy-macos.sh "$(QMAKE)"
else ifneq ($(WINDOWS_HOST),)
deploy: all
	bash scripts/deploy-ucrt64.sh "$(QMAKE)"
else
deploy: all
	bash scripts/deploy-debian.sh "$(DEB_VERSION)"
endif

clean:
	@if test -f $(BUILD_DIR)/Makefile; then \
		$(BUILD_MAKE) -C $(BUILD_DIR) distclean; \
	fi
