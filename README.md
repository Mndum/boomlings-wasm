# Boomlings WASM (work in progress)

Goal: build Boomlings 1.20 from its real game code as WebAssembly, on top of the same cocos2d-x the game used.

The original game was built with cocos2d-x 2.0.1 (`cocos2d-2.0-rc2-x-2.0.1`, found in the binary). This repo
vendors cocos2d-x 2.1.5 (MIT) because it ships an Emscripten platform backend; 2.0.1 and 2.1.5 are close.

## Status
- [x] Milestone 1: cocos2d-x 2.1.5 core builds with a current Emscripten and runs in the browser
  (`src/main.cpp` shows a bitmap-font label and a sprite from the game's sprite sheet, with an action).
- [ ] Milestone 2: port the game classes (`GamePiece`, `GamePieceChanger`, managers, `PlayLayer`, ...) to C++
  from the decompiled code, one class at a time, checked against the JS port.
- [ ] Milestone 3: menus, shop, audio, save data.

## Build
No game assets are included (they belong to RobTop). Extract `assets/` from your own copy of the APK, then:

```
emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DASSETS=/path/to/apk/assets
cmake --build build
python -m http.server -d build   # open boomlings.html
```
Needs the Emscripten SDK and Ninja.

## Notes
- Small fixes made to the vendored backend for modern Emscripten: `arpa/inet.h` include, SDL2 instead of SDL1,
  `-Wno-c++11-narrowing`. TIFF and WebP decoders are not linked (undefined, unused).
- Related: the JS port that is already playable at https://github.com/Mndum/boomlings-web
