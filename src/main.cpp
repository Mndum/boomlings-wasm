// Boomlings on cocos2d-x/Emscripten: the game screen (PlayLayer) running the ported game classes.
#include "cocos2d.h"
#include "PlayLayer.h"
#include <emscripten.h>
USING_NS_CC;

class AppDelegate : public CCApplication {
public:
    bool applicationDidFinishLaunching() {
        CCDirector* d = CCDirector::sharedDirector();
        CCEGLView* v = CCEGLView::sharedOpenGLView();
        d->setOpenGLView(v); d->setContentScaleFactor(2.0f);
        v->setDesignResolutionSize(320, 480, kResolutionShowAll);
        d->setDisplayStats(false); d->setAnimationInterval(1.0 / 60);
        d->runWithScene(PlayLayer::scene());
        return true;
    }
    void applicationDidEnterBackground() { CCDirector::sharedDirector()->pause(); }
    void applicationWillEnterForeground() { CCDirector::sharedDirector()->resume(); }
};

int main(int, char**) {
    // the canvas backing size decides the GL viewport: 2:3 like the game's 320x480 design
    EM_ASM({ var c = Module["canvas"]; c.width = 480; c.height = 720; });
    static AppDelegate app; CCEGLView::sharedOpenGLView(); return CCApplication::sharedApplication()->run(); }
