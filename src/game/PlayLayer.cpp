#include "PlayLayer.h"
#include <deque>
#include <emscripten.h>
USING_NS_CC;

static const int kSpecialIntro[][2] = { {3, kAttrIceBomb}, {5, kAttrPipe}, {7, kAttrSkull}, {9, kAttrMysteryBox}, {11, kAttrStone}, {13, kAttrChanger} };

CCScene* PlayLayer::scene() { CCScene* s = CCScene::create(); s->addChild(PlayLayer::create()); return s; }

int PlayLayer::levelTarget(int level) {
    int limit = level == 0 ? GameTurnManager::basePointsForLen(4) * 20 : (level * 8 + 10) * GameTurnManager::basePointsForLen(4);
    return (int)floorf(limit * GameTurnManager::levelModVal(level));
}

int PlayLayer::countOf(int attr) const {
    int c = 0;
    for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
        AStarNode* n = grid->nodeAt(x, y);
        if (n->piece && n->piece->pieceAttribute() == attr) c++;
    }
    return c;
}

bool PlayLayer::init() {
    if (!CCLayer::init()) return false;
    CCSpriteFrameCache* fc = CCSpriteFrameCache::sharedSpriteFrameCache();
    fc->addSpriteFramesWithFile("ObjectSheet-hd.plist");
    fc->addSpriteFramesWithFile("BoomUISheet-hd.plist");
    AnimLibrary::shared()->load("objectDefinitions.plist");

    addChild(CCLayerColor::create(ccc4(0x72, 0x4a, 0x23, 255)), -10);
    grid = new AStarGrid(N, N);
    for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
        CCSprite* t = CCSprite::createWithSpriteFrameName((x + y) % 2 ? "TileLight01_001.png" : "TileDark01_001.png");
        t->setPosition(posOf(grid->nodeAt(x, y)));
        addChild(t, -5);
    }
    hintLayer = CCNode::create(); addChild(hintLayer, -4);
    boardLayer = CCLayer::create(); addChild(boardLayer, 0);
    layoutHud();

    mgr.level = [this] { return level; };
    mgr.countOf = [this](int a) { return countOf(a); };
    turn = new GameTurnManager(grid);
    turn->level = [this] { return level; };
    turn->onPoints = [this](int p, AStarNode*) { score += p; levelPoints += p; updateHud(); };
    turn->spawnAt = [this](AStarNode* n) { return spawnAt(n, true); };

    scheduleUpdate();
    setTouchEnabled(true);
    startLevel();
    return true;
}

PlayLayer::~PlayLayer() { delete turn; delete grid; }

// PlayLayer::createLayers: HUD sprites at their design positions
void PlayLayer::layoutHud() {
    CCSprite* bottom = CCSprite::createWithSpriteFrameName("bottomBG_001.png");
    bottom->setAnchorPoint(ccp(0.5f, 0)); bottom->setPosition(ccp(160, 0));
    // PlayLayer stretches the panel to start 3.56px above the board's bottom edge
    bottom->setScaleY((960 - 742.64f) / 2 / bottom->getContentSize().height);
    addChild(bottom, 5);
    CCSprite* top = CCSprite::createWithSpriteFrameName("topBar_001.png");
    top->setAnchorPoint(ccp(0.5f, 1)); top->setPosition(ccp(160, 479.75f));
    addChild(top, 5);
    CCSprite* barBG = CCSprite::createWithSpriteFrameName("pointsbar_BG_001.png");
    barBG->setPosition(ccp(160, 91)); addChild(barBG, 6);
    CCSprite* barTop = CCSprite::createWithSpriteFrameName("pointsBar_Top_001.png");
    barTop->setPosition(ccp(160, 91)); addChild(barTop, 8);
    barFill = CCSprite::create("pointsBar-hd.png");
    barFill->setAnchorPoint(ccp(0, 0.5f));
    barFill->setPosition(ccp(160 - barTop->getContentSize().width / 2 + 8, 91));
    addChild(barFill, 7);
    CCSprite* pause = CCSprite::createWithSpriteFrameName("PauseBtn_001.png");
    pause->setPosition(ccp(24, 455.5f)); addChild(pause, 9);
    scoreLbl = CCLabelBMFont::create("0", "numbersFont-hd.fnt");
    scoreLbl->setAnchorPoint(ccp(1, 1)); scoreLbl->setPosition(ccp(273.2f, 462.5f)); addChild(scoreLbl, 9);
    levelLbl = CCLabelBMFont::create("1", "goldFont-hd.fnt");
    levelLbl->setScale(0.8f); levelLbl->setPosition(ccp(66.7f, 451.3f)); addChild(levelLbl, 9);
    bigLbl = CCLabelBMFont::create("", "bigFont-hd.fnt");
    bigLbl->setPosition(ccp(160, 268)); bigLbl->setVisible(false); addChild(bigLbl, 20);
    selectSprite = CCSprite::createWithSpriteFrameName("selectOverlay_001.png");
    selectSprite->setVisible(false); addChild(selectSprite, 1);
}

void PlayLayer::updateHud() {
    char b[32];
    sprintf(b, "%d", score); scoreLbl->setString(b);
    sprintf(b, "%d", level); levelLbl->setString(b);
    float frac = std::max(0.f, std::min(1.f, (float)levelPoints / target));
    float full = 304.f; // inner width of the points bar frame
    CCSize cs = barFill->getContentSize();
    barFill->setVisible(frac > 0);
    barFill->setTextureRect(CCRectMake(0, 0, std::max(10.f, full * frac) * CC_CONTENT_SCALE_FACTOR(), cs.height * CC_CONTENT_SCALE_FACTOR()));
}

void PlayLayer::showBig(const char* text, float dur) {
    bigLbl->setString(text); bigLbl->setVisible(true); bigLbl->setOpacity(255);
    bigLbl->stopAllActions();
    bigLbl->runAction(CCSequence::create(CCDelayTime::create(dur * 0.6f), CCFadeOut::create(dur * 0.4f), NULL));
}

GamePiece* PlayLayer::spawnAt(AStarNode* n, bool animate) {
    mgr.incrementChances();
    GamePiece* p = mgr.createRandomPiece();
    n->piece = p;
    p->setPosition(posOf(n));
    boardLayer->addChild(p);
    if (animate) { p->setScale(0.1f); p->runAction(CCEaseBackOut::create(CCScaleTo::create(0.25f, 1))); }
    return p;
}

void PlayLayer::spawnRandom(int n) {
    std::vector<AStarNode*> free;
    for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) if (!grid->nodeAt(x, y)->piece) free.push_back(grid->nodeAt(x, y));
    for (int i = 0; i < n && !free.empty(); i++) {
        int k = rand() % free.size();
        spawnAt(free[k], true);
        free.erase(free.begin() + k);
    }
}

// GameManager::nextLevel + PlayLayer::startNextLevel
void PlayLayer::startLevel() {
    level++;
    levelPoints = 0; target = levelTarget(level);
    mgr.resetChances();
    turn->timeFrozen = 0;
    char b[32]; sprintf(b, "LEVEL %d", level);
    showBig(b, 1.5f);
    // first appearance of a special on its level
    for (auto& s : kSpecialIntro) if (s[0] == level && countOf(s[1]) < 1) {
        std::vector<AStarNode*> free;
        for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) if (!grid->nodeAt(x, y)->piece) free.push_back(grid->nodeAt(x, y));
        if (!free.empty()) { AStarNode* n = free[rand() % free.size()]; GamePiece* p = GamePiece::create(GamePieceManager::randomPieceColor(level), (PieceAttribute)s[1]); n->piece = p; p->setPosition(posOf(n)); boardLayer->addChild(p); }
    }
    if (!emscripten_run_script_int("location.search.indexOf('empty')>=0"))
    spawnRandom(level == 1 ? 10 : std::min(20, 15 + level / 4));
    phase = kIdle;
    updateHud();
}

// reachable empty cells for the selected piece (flood fill over empty nodes)
void PlayLayer::setSelected(AStarNode* n) {
    selected = n;
    hintLayer->removeAllChildren();
    if (!n) { selectSprite->setVisible(false); return; }
    selectSprite->setVisible(true); selectSprite->setPosition(posOf(n));
    std::set<AStarNode*> seen; std::deque<AStarNode*> q;
    seen.insert(n); q.push_back(n);
    while (!q.empty()) {
        AStarNode* c = q.front(); q.pop_front();
        for (AStarNode* o : c->neighbors) if (!o->piece && !seen.count(o)) { seen.insert(o); q.push_back(o); }
    }
    for (AStarNode* c : seen) {
        if (c == n) continue;
        CCSprite* h = CCSprite::createWithSpriteFrameName("whiteDot_001.png");
        h->setPosition(posOf(c)); h->setOpacity(110); h->setScale(0.7f);
        hintLayer->addChild(h);
    }
}

void PlayLayer::ccTouchesBegan(CCSet* t, CCEvent*) {
    if (phase != kIdle) return;
    CCTouch* tc = (CCTouch*)*t->begin();
    CCPoint p = convertToNodeSpace(tc->getLocation());
    AStarNode* n = grid->nodeAtPoint(p.x, p.y, cell, ox, oy);
    if (!n) return;
    if (n->piece) { if (n->piece->frozen || n == selected) { setSelected(NULL); return; } setSelected(n); return; }
    if (selected) movePiece(selected, n);
}

void PlayLayer::movePiece(AStarNode* from, AStarNode* to) {
    std::vector<AStarNode*> path;
    if (!grid->findPath(from, to, path)) return;
    GamePiece* pc = from->piece;
    setSelected(NULL);
    CCArray* acts = CCArray::create();
    for (size_t i = 1; i < path.size(); i++) acts->addObject(CCMoveTo::create(0.04f, posOf(path[i])));
    acts->addObject(CCCallFuncN::create(this, callfuncN_selector(PlayLayer::landed)));
    from->piece = NULL; to->piece = pc; landing = to; phase = kMoving;
    pc->runAction(CCSequence::create(acts));
}

void PlayLayer::landed(CCNode*) {
    turn->pieceLanded(landing);
    phase = kResolving;
}

void PlayLayer::endTurn() {
    if (levelPoints >= target) {
        showBig("LEVEL COMPLETE!", 1.6f);
        for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
            AStarNode* n = grid->nodeAt(x, y);
            if (n->piece) { n->piece->removeFromParent(); n->piece = NULL; }
        }
        phase = kLevelOver; levelTimer = 1.6f;
        return;
    }
    if (turn->timeFrozen > 0) turn->timeFrozen--;
    else {
        turn->updateTurnSensitive();
        spawnRandom(level < 4 ? 2 : 3);
    }
    if (turn->skullDeath) { showBig("A SKULL BLEW UP", 99); phase = kDead; return; }
    if (!grid->validMoveExists()) { showBig("NO MOVES LEFT", 99); phase = kDead; return; }
    phase = kIdle;
}

void PlayLayer::update(float dt) {
    turn->tick(dt);
    if (phase == kResolving && !turn->busy()) endTurn();
    else if (phase == kLevelOver) { levelTimer -= dt; if (levelTimer <= 0) startLevel(); }
}
