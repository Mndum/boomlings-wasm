// Milestone 1: bring up cocos2d-x 2.x on Emscripten with the game's own bitmap font and sprite sheet.
#include "cocos2d.h"
USING_NS_CC;

class TestLayer : public CCLayerColor {
public:
    static CCScene* scene() { CCScene* s = CCScene::create(); s->addChild(TestLayer::create()); return s; }
    CREATE_FUNC(TestLayer);
    bool init() {
        if (!CCLayerColor::initWithColor(ccc4(20, 60, 140, 255))) return false;
        CCSize w = CCDirector::sharedDirector()->getWinSize();
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("BoomUISheet.plist");
        CCLabelBMFont* t = CCLabelBMFont::create("BOOMLINGS WASM", "bigFont.fnt");
        t->setPosition(ccp(w.width / 2, w.height / 2 + 60)); addChild(t);
        CCSprite* s = CCSprite::createWithSpriteFrameName("shareBtn_001.png");
        if (s) { s->setPosition(ccp(w.width / 2, w.height / 2 - 40)); addChild(s); s->runAction(CCRepeatForever::create(CCSequence::create(CCScaleTo::create(0.5f, 1.3f), CCScaleTo::create(0.5f, 1.0f), NULL))); }
        return true;
    }
};

class AppDelegate : public CCApplication {
public:
    bool applicationDidFinishLaunching() {
        CCDirector* d = CCDirector::sharedDirector();
        CCEGLView* v = CCEGLView::sharedOpenGLView();
        d->setOpenGLView(v);
        v->setDesignResolutionSize(320, 480, kResolutionShowAll);
        d->setDisplayStats(false); d->setAnimationInterval(1.0 / 60);
        d->runWithScene(TestLayer::scene());
        return true;
    }
    void applicationDidEnterBackground() { CCDirector::sharedDirector()->pause(); }
    void applicationWillEnterForeground() { CCDirector::sharedDirector()->resume(); }
};

int main(int, char**) { static AppDelegate app; CCEGLView::sharedOpenGLView(); return CCApplication::sharedApplication()->run(); }
