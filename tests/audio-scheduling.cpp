// Run with `make test-audio`s
// SDL's dummy device stays paused; tests drive the real renderer themselves.
#include "emulation/SoundSDL2.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// Some SDL2 pkg-config files define main=SDL_main for GUI applications.
#undef main

using namespace Emulation;

class TestSound : public SoundSDL2 {
public:
    explicit TestSound(TIASound* tia) : SoundSDL2(tia) {
        open();
        mute(true);
        reset();
    }

    std::vector<Int16> render(unsigned samples) {
        std::vector<Int16> result(samples * 2);
        processFragment(result.data(), result.size());
        return result;
    }

    void frame(int left, int right) {
        set(AUDV0, left);
        set(AUDV1, right);
        endFrame();
    }
};

static const unsigned lead = 2048; // Two 1024-sample dummy-device buffers.

static void checkFrames(float rate, unsigned interval) {
    TIASound tia;
    TestSound sound(&tia);
    sound.setFrameRate(rate);
    sound.frame(3, 4);
    sound.frame(7, 8); // Simulate several timer ticks arriving together.
    sound.frame(0, 0);

    auto silence = sound.render(lead);
    assert(std::all_of(silence.begin(), silence.end(), [](Int16 x) { return x == 0; }));
    assert(tia.get(AUDV0) == 0);
    sound.render(1);
    assert(tia.get(AUDV0) == 3 && tia.get(AUDV1) == 4);
    sound.render(interval - 1);
    assert(tia.get(AUDV0) == 3);
    sound.render(1);
    assert(tia.get(AUDV0) == 7 && tia.get(AUDV1) == 8);
    sound.render(interval - 1);
    assert(tia.get(AUDV0) == 7);
    sound.render(1);
    assert(tia.get(AUDV0) == 0 && tia.get(AUDV1) == 0);

    // After a long producer stall, catch-up frames must still be separated.
    sound.render(44100);
    sound.frame(9, 10);
    sound.frame(11, 12);
    sound.render(lead);
    assert(tia.get(AUDV0) == 0);
    sound.render(interval);
    assert(tia.get(AUDV0) == 9 && tia.get(AUDV1) == 10);
    sound.render(1);
    assert(tia.get(AUDV0) == 11 && tia.get(AUDV1) == 12);
}

static std::vector<Int16> renderSequence(unsigned chunk) {
    TIASound tia;
    TestSound sound(&tia);
    sound.setFrameRate(60);
    // Exercise queue growth and multiple changes within callback buffers.
    for(int i = 0; i < 200; ++i)
        sound.frame(i % 16, (i * 3) % 16);
    std::vector<Int16> result;
    for(unsigned remaining = lead + 200 * 735; remaining;) {
        unsigned count = std::min(chunk, remaining);
        auto part = sound.render(count);
        result.insert(result.end(), part.begin(), part.end());
        remaining -= count;
    }
    return result;
}

static void checkRateChangeAndRestart() {
    TIASound tia;
    TestSound sound(&tia);
    sound.setFrameRate(50);
    sound.frame(3, 4);
    sound.frame(5, 6);
    sound.close(); // Player::setTVStandard closes, changes rate, then opens.
    sound.setFrameRate(60);
    sound.open();
    sound.mute(true);
    sound.reset();
    sound.frame(7, 8);
    sound.frame(0, 0); // Stop.
    sound.frame(9, 10); // Start again.
    sound.render(lead);
    assert(tia.get(AUDV0) == 0);
    sound.render(735);
    assert(tia.get(AUDV0) == 7);
    sound.render(735);
    assert(tia.get(AUDV0) == 0);
    sound.render(1);
    assert(tia.get(AUDV0) == 9);
}

int main() {
    SDL_SetMainReady();
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << SDL_GetError() << '\n';
        return 1;
    }
    checkFrames(50, 882);
    checkFrames(60, 735);
    const auto expected = renderSequence(1);
    assert(expected == renderSequence(511));
    assert(expected == renderSequence(1024));
    assert(expected == renderSequence(4096));
    checkRateChangeAndRestart();
    SDL_Quit();
    std::cout << "Audio scheduling tests passed\n";
}
