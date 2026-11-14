cmake_minimum_required(VERSION 3.16)
project(boomlings_wasm C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS ON)
set(CC third_party/cocos2dx)
file(STRINGS cocos_sources.txt COCOS_SRC)
list(TRANSFORM COCOS_SRC PREPEND ${CC}/)
add_library(cocos2d STATIC ${COCOS_SRC})
target_compile_definitions(cocos2d PUBLIC EMSCRIPTEN NDEBUG CP_USE_DOUBLES=0 __CC_PLATFORM_FILEUTILS_CPP__)
target_include_directories(cocos2d PUBLIC
  ${CC} ${CC}/cocoa ${CC}/include ${CC}/kazmath/include ${CC}/platform/emscripten
  ${CC}/platform/third_party/emscripten/libpng ${CC}/platform/third_party/emscripten/libz
  ${CC}/platform/third_party/emscripten/libtiff/include ${CC}/platform/third_party/emscripten/libjpeg
  ${CC}/platform/third_party/emscripten/libwebp ${CC}/platform/third_party/linux
  ${CC}/platform/third_party/linux/libfreetype2 third_party/extensions third_party/external/chipmunk/include/chipmunk)
target_compile_options(cocos2d PRIVATE -w -Wno-c++11-narrowing -sUSE_SDL=2 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -sUSE_FREETYPE=1)

add_executable(boomlings src/main.cpp src/game/AnimLibrary.cpp src/game/CCAnimatedSprite.cpp src/game/GamePiece.cpp)
target_include_directories(boomlings PRIVATE src/game)
target_link_libraries(boomlings cocos2d)
set(ASSETS "" CACHE PATH "folder with the APK's assets (not part of the repo)")
target_compile_options(boomlings PRIVATE -w -sUSE_SDL=2)
target_link_options(boomlings PRIVATE -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=["png"] -sUSE_SDL_MIXER=2 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -sUSE_FREETYPE=1 -sFULL_ES2=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=268435456 -sWASM=1 -sERROR_ON_UNDEFINED_SYMBOLS=0)
if(ASSETS)
  target_link_options(boomlings PRIVATE "--preload-file" "${ASSETS}@/")
endif()
set_target_properties(boomlings PROPERTIES SUFFIX ".html")
