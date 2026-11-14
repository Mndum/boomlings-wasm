// Milestone 1: bring up cocos2d-x 2.x on Emscripten with the game's own bitmap font and sprite sheet.
#include "cocos2d.h"
USING_NS_CC;

#include "GamePiece.h"
#include "AStarGrid.h"
#include "GamePieceManager.h"
// Milestone 2 test board: 8x8 AStarGrid filled by GamePieceManager. Tap a piece to select it, tap an empty cell to walk it there
// along AStarGrid::findPath; the connected same-colour group of the landing cell bumps.
class TestLayer : public CCLayerColor {
public:
    static CCScene* scene() { CCScene* s = CCScene::create(); s->addChild(TestLayer::create()); return s; }
    CREATE_FUNC(TestLayer);
    AStarGrid* grid = NULL;
    GamePieceManager mgr;
    AStarNode* selected = NULL;
    bool busy = false;
    static const int N = 8;
    float cell = 38, ox = 8, oy = 90;
    CCPoint posOf(AStarNode* n) { return ccp(ox + (n->gx + 0.5f) * cell, oy + (n->gy + 0.5f) * cell); }
    bool init() {
        if (!CCLayerColor::initWithColor(ccc4(114, 74, 35, 255))) return false;
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("ObjectSheet-hd.plist");
        AnimLibrary::shared()->load("objectDefinitions.plist");
        grid = new AStarGrid(N, N);
        mgr.level = [] { return 8; };
        mgr.countOf = [this](int a) { int c = 0; for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) { AStarNode* n = grid->nodeAt(x, y); if (n->piece && n->piece->pieceAttribute() == a) c++; } return c; };
        for (int i = 0; i < 18; i++) {
            AStarNode* n = grid->nodeAt(rand() % N, rand() % N);
            if (n->piece) continue;
            mgr.incrementChances();
            n->piece = mgr.createRandomPiece();
            n->piece->setPosition(posOf(n)); n->piece->setScale(0.75f);
            addChild(n->piece);
        }
        setTouchEnabled(true);
        return true;
    }
    ~TestLayer() { delete grid; }
    void ccTouchesBegan(CCSet* t, CCEvent*) {
        if (busy) return;
        CCTouch* tc = (CCTouch*)*t->begin();
        CCPoint p = convertToNodeSpace(tc->getLocation());
        AStarNode* n = grid->nodeAtPoint(p.x, p.y, cell, ox, oy);
        if (!n) return;
        if (n->piece) { if (selected && selected->piece) selected->piece->setScale(0.75f); selected = n; n->piece->setScale(0.9f); return; }
        if (!selected) return;
        std::vector<AStarNode*> path;
        if (!grid->findPath(selected, n, path)) return;
        GamePiece* pc = selected->piece;
        pc->setScale(0.75f);
        CCArray* acts = CCArray::create();
        for (size_t i = 1; i < path.size(); i++) acts->addObject(CCMoveTo::create(0.08f, posOf(path[i])));
        acts->addObject(CCCallFuncN::create(this, callfuncN_selector(TestLayer::landed)));
        selected->piece = NULL; n->piece = pc; landing = n; selected = NULL; busy = true;
        pc->runAction(CCSequence::create(acts));
    }
    AStarNode* landing = NULL;
    void landed(CCNode*) {
        busy = false;
        for (AStarNode* g : grid->sameTypeConnected(landing))
            g->piece->runAction(CCSequence::create(CCScaleTo::create(0.1f, 1.0f), CCScaleTo::create(0.1f, 0.75f), NULL));
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
