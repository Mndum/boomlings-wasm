// Sound effects and music through the browser (HTMLAudio), reading the OGG/MP3 files from the preloaded assets.
// Same behaviour as the JS port's AudioMan: one clone per effect, volumes as measured against the original.
#pragma once
namespace Sfx {
void play(const char* file, float volume = 1);
void music(const char* file, float volume = 0.45f);
void stopMusic();
void preload(const char* const* files, int n);   // decode effects up front (Web Audio) so the first play does not hitch
void setVolumes(float music, float sfx);   // PauseLayer sliders (0..1), persisted in localStorage
float musicVolume();
float sfxVolume();
}
