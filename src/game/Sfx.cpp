#include "Sfx.h"
#include <emscripten.h>

EM_JS(void, js_sfx, (const char* file, float vol), {
    var n = UTF8ToString(file);
    var S = Module.__sfx || (Module.__sfx = { urls: {}, music: null, musicName: null, mv: 1, sv: 1 });
    try {
        if (!S.urls[n]) {
            var data = FS.readFile('/' + n);
            S.urls[n] = URL.createObjectURL(new Blob([data], { type: n.endsWith('.mp3') ? 'audio/mpeg' : 'audio/ogg' }));
        }
        var a = new Audio(S.urls[n]);
        a.volume = Math.min(1, vol * S.sv);
        a.play().catch(function() {});
    } catch (e) {}
});

EM_JS(void, js_music, (const char* file, float vol), {
    var n = UTF8ToString(file);
    var S = Module.__sfx || (Module.__sfx = { urls: {}, music: null, musicName: null, mv: 1, sv: 1 });
    try {
        if (S.musicName === n && S.music && !S.music.paused) return;
        if (S.music) { S.music.pause(); S.music = null; }
        S.musicName = n;
        if (!S.urls[n]) {
            var data = FS.readFile('/' + n);
            S.urls[n] = URL.createObjectURL(new Blob([data], { type: n.endsWith('.mp3') ? 'audio/mpeg' : 'audio/ogg' }));
        }
        var a = new Audio(S.urls[n]);
        a.loop = true; a.baseVol = vol; a.volume = Math.min(1, vol * S.mv);
        S.music = a;
        var go = function() { a.play().catch(function() {}); };
        go();
        // browsers block audio until the first gesture: retry on it
        if (!S.hooked) { S.hooked = true; var f = function() { if (S.music && S.music.paused) S.music.play().catch(function() {}); }; document.addEventListener('pointerdown', f); document.addEventListener('keydown', f); }
    } catch (e) {}
});

EM_JS(void, js_stop_music, (), {
    var S = Module.__sfx;
    if (S && S.music) { S.music.pause(); S.music = null; S.musicName = null; }
});

EM_JS(void, js_volumes, (float mv, float sv), {
    var S = Module.__sfx || (Module.__sfx = { urls: {}, music: null, musicName: null, mv: 1, sv: 1 });
    S.mv = mv; S.sv = sv;
    if (S.music) S.music.volume = Math.min(1, (S.music.baseVol || 0.45) * mv);
    try { localStorage.setItem('boomlings.mv', mv); localStorage.setItem('boomlings.sv', sv); } catch (e) {}
});
EM_JS(float, js_get_vol, (int which), {
    var S = Module.__sfx || (Module.__sfx = { urls: {}, music: null, musicName: null, mv: 1, sv: 1 });
    try { var v = localStorage.getItem(which ? 'boomlings.sv' : 'boomlings.mv'); if (v !== null) { if (which) S.sv = +v; else S.mv = +v; } } catch (e) {}
    return which ? S.sv : S.mv;
});

namespace Sfx {
void setVolumes(float m, float s) { js_volumes(m, s); }
float musicVolume() { return js_get_vol(0); }
float sfxVolume() { return js_get_vol(1); }
void play(const char* file, float volume) { js_sfx(file, volume); }
void music(const char* file, float volume) { js_music(file, volume); }
void stopMusic() { js_stop_music(); }
}
