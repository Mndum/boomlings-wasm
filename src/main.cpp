// Boomlings on cocos2d-x/Emscripten: the game screen (PlayLayer) running the ported game classes.
#include "cocos2d.h"
#include "PlayLayer.h"
#include "GamePiece.h"
#include <emscripten.h>
USING_NS_CC;

extern GamePiece* g_dbgPiece;
extern "C" EMSCRIPTEN_KEEPALIVE int dbg_idx() { return g_dbgPiece ? g_dbgPiece->frameIndex() : -1; }
// deterministic stepping for pixel/animation comparisons (?manual): advance the scheduler n fixed 1/60s frames, then redraw
extern "C" EMSCRIPTEN_KEEPALIVE void step(int n) {
    CCDirector* d = CCDirector::sharedDirector();
    for (int i = 0; i < n; i++) d->getScheduler()->update(1.0f / 60);
    d->drawScene();
}

class AppDelegate : public CCApplication {
public:
    bool applicationDidFinishLaunching() {
        CCDirector* d = CCDirector::sharedDirector();
        CCEGLView* v = CCEGLView::sharedOpenGLView();
        d->setOpenGLView(v); d->setContentScaleFactor(2.0f);
        v->setDesignResolutionSize(320, 480, kResolutionShowAll);
        d->setDisplayStats(false); d->setAnimationInterval(1.0 / 60);
        d->runWithScene(PlayLayer::scene());
        if (emscripten_run_script_int("location.search.indexOf('manual')>=0")) d->pause();
        return true;
    }
    void applicationDidEnterBackground() { CCDirector::sharedDirector()->pause(); }
    void applicationWillEnterForeground() { CCDirector::sharedDirector()->resume(); }
};

// stop simulating while the tab is hidden and resume without a huge delta (no jump when coming back)
extern "C" EMSCRIPTEN_KEEPALIVE void set_hidden(int hidden) {
    CCDirector* d = CCDirector::sharedDirector();
    if (hidden) d->pause(); else d->resume();
}

int main(int, char**) {
    // the canvas backing size decides the GL viewport: 2:3 like the game's 320x480 design
    EM_ASM({ var c = Module["canvas"]; c.width = 640; c.height = 960;
        if (location.search.indexOf('manual') < 0) document.addEventListener('visibilitychange', function() { Module._set_hidden(document.hidden ? 1 : 0); }); });
    static AppDelegate app; CCEGLView::sharedOpenGLView(); return CCApplication::sharedApplication()->run(); }
