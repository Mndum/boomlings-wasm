// Milestone 1: bring up cocos2d-x 2.x on Emscripten with the game's own bitmap font and sprite sheet.
#include "cocos2d.h"
USING_NS_CC;

#include "GamePiece.h"
class TestLayer : public CCLayerColor {
public:
    static CCScene* scene() { CCScene* s = CCScene::create(); s->addChild(TestLayer::create()); return s; }
    CREATE_FUNC(TestLayer);
    std::vector<GamePiece*> changers;
    bool init() {
        if (!CCLayerColor::initWithColor(ccc4(114, 74, 35, 255))) return false;
        CCSize w = CCDirector::sharedDirector()->getWinSize();
        CCSpriteFrameCache* fc = CCSpriteFrameCache::sharedSpriteFrameCache();
        fc->addSpriteFramesWithFile("ObjectSheet-hd.plist");
        AnimLibrary::shared()->load("objectDefinitions.plist");
        // one row per attribute, one column per colour (frame data from the game's own animation descriptions)
        const PieceAttribute attrs[] = { kAttrBlock, kAttrBomb, kAttrColor, kAttrChanger };
        for (int r = 0; r < 4; r++)
            for (int c = 1; c <= 6; c++) {
                GamePiece* p = GamePiece::create((PieceColor)c, attrs[r]);
                if (!p) continue;
                p->setPosition(ccp(30 + (c - 1) * 52, w.height - 60 - r * 70));
                addChild(p);
                if (attrs[r] == kAttrChanger) changers.push_back(p);
            }
        schedule(schedule_selector(TestLayer::cycle), 1.2f);
        return true;
    }
    // GamePieceChanger::updateTurnSensitive: pick another colour, keep the Changer attribute
    void cycle(float) {
        for (GamePiece* p : changers) {
            int c = p->pieceColor();
            for (int i = 0; i < 10 && c == p->pieceColor(); i++) c = 1 + rand() % 6;
            p->changePiece((PieceColor)c, kAttrChanger);
        }
    }
};

class AppDelegate : public CCApplication {
public:
    bool applicationDidFinishLaunching() {
        CCDirector* d = CCDirector::sharedDirector();
        CCEGLView* v = CCEGLView::sharedOpenGLView();
        d->setOpenGLView(v); d->setContentScaleFactor(2.0f);
        v->setDesignResolutionSize(320, 480, kResolutionShowAll);
        d->setDisplayStats(false); d->setAnimationInterval(1.0 / 60);
        d->runWithScene(TestLayer::scene());
        return true;
    }
    void applicationDidEnterBackground() { CCDirector::sharedDirector()->pause(); }
    void applicationWillEnterForeground() { CCDirector::sharedDirector()->resume(); }
};

int main(int, char**) { static AppDelegate app; CCEGLView::sharedOpenGLView(); return CCApplication::sharedApplication()->run(); }
