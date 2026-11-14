// Sound effects and music through the browser (HTMLAudio), reading the OGG/MP3 files from the preloaded assets.
// Same behaviour as the JS port's AudioMan: one clone per effect, volumes as measured against the original.
#pragma once
namespace Sfx {
void play(const char* file, float volume = 1);
void music(const char* file, float volume = 0.45f);
void stopMusic();
}
