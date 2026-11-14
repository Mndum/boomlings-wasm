#include "PlayLayer.h"
#include "Sfx.h"
#include <deque>
#include <map>
#include <emscripten.h>
USING_NS_CC;
GamePiece* g_dbgPiece = NULL;

// scissors its children to the region below the board: PlayLayer clips the bottom panel 10px under the board's edge
class PanelClip : public CCNode {
public:
    float height = 0;
    virtual void visit() {
        glEnable(GL_SCISSOR_TEST);
        CCEGLView::sharedOpenGLView()->setScissorInPoints(0, 0, 320, height);
        CCNode::visit();
        glDisable(GL_SCISSOR_TEST);
    }
};

static const int kSpecialIntro[][2] = { {3, kAttrIceBomb}, {5, kAttrPipe}, {7, kAttrSkull}, {9, kAttrMysteryBox}, {11, kAttrStone}, {13, kAttrChanger} };

// radial glow of the bomb flash: white (alpha a) in the centre fading to transparent orange at the rim
class GlowNode : public CCNode {
public:
    float radius = 1, alpha = 1;
    bool init() { if (!CCNode::init()) return false; setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionColor)); return true; }
    CREATE_FUNC(GlowNode);
    virtual void draw() {
        CC_NODE_DRAW_SETUP();
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_Color);
        const int N = 32;
        ccVertex2F v[N + 2]; ccColor4F c[N + 2];
        v[0].x = 0; v[0].y = 0; c[0] = ccc4f(1, 1, 1, 0.8f * alpha);
        for (int i = 0; i <= N; i++) { float a = i * 2 * 3.14159265f / N; v[i + 1].x = cosf(a) * radius; v[i + 1].y = sinf(a) * radius; c[i + 1] = ccc4f(1, 200 / 255.f, 80 / 255.f, 0); }
        glBindBuffer(GL_ARRAY_BUFFER, 0); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, v);
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, 0, c);
        glDrawArrays(GL_TRIANGLE_FAN, 0, N + 2);
    }
};

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
    fc->addSpriteFramesWithFile("particleImgSheet.plist");
    AnimLibrary::shared()->load("objectDefinitions.plist");

    addChild(CCLayerColor::create(ccc4(0x72, 0x4a, 0x23, 255)), -10);
    grid = new AStarGrid(N, N);
    for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
        CCSprite* t = CCSprite::createWithSpriteFrameName((x + y) % 2 ? "TileLight01_001.png" : "TileDark01_001.png");
        t->setPosition(posOf(grid->nodeAt(x, y)));
        addChild(t, -5);
    }
    hintLayer = CCNode::create(); addChild(hintLayer, -4);
    CCNode* shadows = CCNode::create(); addChild(shadows, -3); GamePiece::shadowLayer = shadows;
    boardLayer = CCLayer::create(); addChild(boardLayer, 0);
    trailLayer = CCNode::create(); addChild(trailLayer, -2);
    fxLayer = CCNode::create(); addChild(fxLayer, 3);
    layoutHud();

    mgr.level = [this] { return level; };
    mgr.countOf = [this](int a) { return countOf(a); };
    turn = new GameTurnManager(grid);
    turn->level = [this] { return level; };
    turn->onPoints = [this](int p, AStarNode* at, int color) { score += p; levelPoints += p; updateHud(); addScore(p, at, color); };
    turn->onEffect = [this](const char* k, AStarNode* at, int color, int radius, AStarNode* to) { addEffect(k, at, color, radius, to); };
    turn->onSound = [](const char* f, float v) { Sfx::play(f, v); };
    turn->spawnAt = [this](AStarNode* n) { return spawnAt(n, true); };

    Sfx::music("BoomlingsLoop.mp3", 0.45f);
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
    // PlayLayer stretches the panel from 3.56px above the board's bottom edge and clips it at 10px below it; the board casts a shadow on its last row
    {
        const float topPx = 743.64f, clipPx = 755.27f;
        CCSize cs = bottom->getContentSize();
        float scaleY = (960 - topPx) / 2 / cs.height;
        bottom->setScaleY(scaleY);
        CCLayerGradient* sh = CCLayerGradient::create(ccc4(20, 10, 0, 0), ccc4(20, 10, 0, 179), ccp(0, -1));
        sh->setContentSize(CCSizeMake(320, 8.8f)); sh->setPosition(ccp(0, (960 - clipPx) / 2));
        addChild(sh, 4);
    }
    PanelClip* clip = new PanelClip(); clip->autorelease(); clip->height = (960 - 755.27f) / 2;
    clip->addChild(bottom); addChild(clip, 5);
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
    if (emscripten_run_script_int("location.search.indexOf('manual')>=0")) return;
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
    if (animate) {
        p->startEnter();
        switch (p->pieceAttribute()) {
            case kAttrSkull: Sfx::play("skullTest.ogg", 0.7f); break;
            case kAttrMysteryBox: Sfx::play("mysteryAppear.ogg", 0.8f); break;
            case kAttrPipe: Sfx::play("pipeenter02.ogg", 0.7f); break;
            case kAttrMegaBomb: case kAttrMegaColor: Sfx::play("megaEnter.ogg", 0.7f); break;
            default: break;
        }
    }
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
    Sfx::play("lvlup003.ogg", 0.8f);
    // first appearance of a special on its level
    for (auto& s : kSpecialIntro) if (s[0] == level && countOf(s[1]) < 1) {
        std::vector<AStarNode*> free;
        for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) if (!grid->nodeAt(x, y)->piece) free.push_back(grid->nodeAt(x, y));
        if (!free.empty()) { AStarNode* n = free[rand() % free.size()]; GamePiece* p = GamePiece::create(GamePieceManager::randomPieceColor(level), (PieceAttribute)s[1]); n->piece = p; p->setPosition(posOf(n)); boardLayer->addChild(p); }
    }
    if (emscripten_run_script_int("location.search.indexOf('pieces')>=0")) {
        // pixel-comparison fixture: attribute per column (rows 0-5 = colours 1-6), extras in the top two rows
        const int cols[8] = { kAttrBlock, kAttrBomb, kAttrColor, kAttrMegaBomb, kAttrMegaColor, kAttrIceBomb, kAttrPipe, kAttrSkull };
        for (int x = 0; x < 8; x++) for (int c = 1; c <= 6; c++) { AStarNode* n = grid->nodeAt(x, 7 - (c - 1)); GamePiece* g = GamePiece::create((PieceColor)c, (PieceAttribute)cols[x]); n->piece = g; g->setPosition(posOf(n)); boardLayer->addChild(g); }
        for (int c = 1; c <= 6; c++) {
            AStarNode* n = grid->nodeAt(c - 1, 1); GamePiece* g = GamePiece::create((PieceColor)c, kAttrMysteryBox); n->piece = g; g->setPosition(posOf(n)); boardLayer->addChild(g);
            n = grid->nodeAt(c - 1, 0); g = GamePiece::create((PieceColor)c, kAttrChanger); n->piece = g; g->setPosition(posOf(n)); boardLayer->addChild(g);
        }
        for (int x = 6; x < 8; x++) { AStarNode* n = grid->nodeAt(x, 1); GamePiece* g = GamePiece::create(kColorGreen, kAttrStone); n->piece = g; g->setPosition(posOf(n)); boardLayer->addChild(g); }
        g_dbgPiece = grid->nodeAt(0, 7)->piece;
        char* clip = emscripten_run_script_string("(location.search.match(/clip=([A-Za-z0-9]+)/)||[0,''])[1]");
        if (clip && *clip) for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) { AStarNode* n = grid->nodeAt(x, y); if (n->piece) n->piece->runAnimation(clip, false); }
    } else if (emscripten_run_script_int("location.search.indexOf('fx')>=0")) {
        // effect comparison fixture (JS rows y -> gy = 7 - y)
        auto N_ = [this](int x, int jy) { return grid->nodeAt(x, 7 - jy); };
        addEffect("cloud", N_(0, 1), 0, 1, NULL); addEffect("splash", N_(2, 1), kColorGreen, 1, NULL);
        addEffect("flash", N_(4, 1), kColorRed, 1, NULL); addEffect("iceblast", N_(6, 1), 0, 1, NULL);
        addEffect("ball", N_(0, 4), kColorBlue, 1, N_(3, 4)); addEffect("flash", N_(6, 4), kColorYellow, 2, NULL);
        level = 1; addScore(20, N_(1, 6), kColorOrange); addScore(250, N_(5, 6), kColorBlue);
    } else
    if (!emscripten_run_script_int("location.search.indexOf('empty')>=0"))
    spawnRandom(level == 1 ? 10 : std::min(20, 15 + level / 4));
    phase = kIdle;
    updateHud();
}

// reachable empty cells for the selected piece (flood fill over empty nodes)
void PlayLayer::setSelected(AStarNode* n) {
    if (n) Sfx::play("click01.ogg", 0.5f); else if (selected) Sfx::play("click05.ogg", 0.3f);
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
    for (size_t i = 1; i < path.size(); i++) acts->addObject(CCMoveTo::create(0.08f, posOf(path[i])));
    acts->addObject(CCCallFuncN::create(this, callfuncN_selector(PlayLayer::landed)));
    from->piece = NULL; to->piece = pc; landing = to; phase = kMoving;
    pc->runAction(CCSequence::create(acts));
    // PlayLayer::attachTrailToPiece: trailEffect tinted with the piece's strong colour, emitted in world space under the pieces
    static const float strong[7][3] = { {1,1,1}, {100/255.f,1,80/255.f}, {1,110/255.f,150/255.f}, {1,165/255.f,75/255.f}, {100/255.f,165/255.f,1}, {1,110/255.f,250/255.f}, {1,1,100/255.f} };
    int c = pc->pieceColor();
    trail = spawnParticles("trailEffect", pc->getPosition() + ccp(0, 5), 0, strong[c >= 0 && c <= 6 ? c : 0], trailLayer);
    trailPiece = pc;
}

void PlayLayer::landed(CCNode*) {
    if (trail) { trail->stopSystem(); trail = NULL; }
    trailPiece = NULL;
    turn->pieceLanded(landing);
    phase = kResolving;
}

void PlayLayer::endTurn() {
    if (levelPoints >= target) {
        showBig("LEVEL COMPLETE!", 1.6f);
        Sfx::play("levelComplete002.ogg", 0.9f);
        spawnParticles("levelComplete01", ccp(160, 107.4f), 6); spawnParticles("levelComplete02", ccp(160, 107.4f), 6);
        for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
            AStarNode* n = grid->nodeAt(x, y);
            if (n->piece) { n->piece->removeFromParent(); n->piece = NULL; Sfx::play(rand() % 2 ? "pop2b2.ogg" : "pops2a.ogg", 0.4f); }
        }
        phase = kLevelOver; levelTimer = 1.6f;
        return;
    }
    if (turn->timeFrozen > 0) turn->timeFrozen--;
    else {
        turn->updateTurnSensitive();
        spawnRandom(level < 4 ? 2 : 3);
    }
    if (turn->skullDeath) { Sfx::stopMusic(); Sfx::play("skullEnding002.ogg", 1); showBig("A SKULL BLEW UP", 99); phase = kDead; return; }
    if (!grid->validMoveExists()) { Sfx::stopMusic(); Sfx::play("res02.ogg", 0.8f); showBig("NO MOVES LEFT", 99); phase = kDead; return; }
    phase = kIdle;
}

void PlayLayer::update(float dt) {
    turn->tick(dt);
    if (trail && trailPiece) trail->setPosition(trailPiece->getPosition() + ccp(0, 5));
    updateWarns();
    updateEffects(dt);
    if (phase == kResolving && !turn->busy()) endTurn();
    else if (phase == kLevelOver) { levelTimer -= dt; if (levelTimer <= 0) startLevel(); }
}

// Skull_w_* / Ice_w_* glow on tickers that are about to detonate (PlayLayer warn effects)
void PlayLayer::updateWarns() {
    std::map<GamePiece*, int> want;
    for (int x = 0; x < N; x++) for (int y = 0; y < N; y++) {
        GamePiece* p = grid->nodeAt(x, y)->piece;
        if (!p) continue;
        PieceAttribute a = p->pieceAttribute();
        if ((a == kAttrSkull && p->tick <= 3) || (a == kAttrIceBomb && p->tick <= 2)) want[p] = p->tick;
    }
    for (auto it = warns.begin(); it != warns.end();) {
        auto w = want.find(it->first);
        if (w == want.end() || w->second != it->second.tick) { it->second.ps->stopSystem(); it->second.ps->release(); it = warns.erase(it); }
        else { it->second.ps->setPosition(it->first->getPosition()); ++it; }
    }
    for (auto& w : want) {
        if (warns.count(w.first)) continue;
        GamePiece* p = w.first; int t = w.second;
        char nm[24];
        if (p->pieceAttribute() == kAttrSkull) sprintf(nm, "Skull_w_0%d", t <= 1 ? 3 : t <= 2 ? 2 : 1);
        else sprintf(nm, "Ice_w_0%d", t <= 1 ? 2 : 1);
        CCParticleSystemQuad* ps = spawnParticles(nm, p->getPosition(), 4);
        if (ps) { ps->retain(); warns[p] = { ps, t }; }
    }
}

static const char* kScoreColors[7] = { "", "69ff69", "ff6464", "ffa63c", "64a5ff", "ff78ff", "fff03c" };

void PlayLayer::addEffect(const char* kind, AStarNode* at, int color, int radius, AStarNode* to) {
    Fx e; e.kind = kind; e.t = 0; e.radius = radius; e.n = 1; e.node = NULL;
    e.a = posOf(at); e.b = to ? posOf(to) : e.a;
    std::string tag = GamePiece::colorToString(color);
    CCSprite* s = NULL;
    if (e.kind == "splash") { e.dur = 6 * 0.0833f; e.base = "Splash_" + tag; e.n = 6; }
    else if (e.kind == "cloud") { e.dur = 7 * 0.0833f; e.base = "Cloud"; e.n = 7; }
    else if (e.kind == "flash") { e.dur = 0.45f; e.base = "FlashEffect"; e.n = 6; }
    else if (e.kind == "iceblast") { e.dur = 0.5f; e.base = "IceBlast"; e.n = 1; }
    else if (e.kind == "ball") { e.dur = 0.3f; e.base = "Ball_" + tag; e.n = 1; }
    else if (e.kind == "charge") { CCParticleSystemQuad* ps = spawnParticles("chargeEffect", e.a, 4); if (ps && radius > 1) ps->setScale((float)radius); return; }
    else if (e.kind == "stonehit") { spawnParticles("stoneHit", e.a, 4); return; }
    else return;
    CCNode* holder = CCNode::create();
    holder->setPosition(e.a);
    if (e.kind == "flash") { GlowNode* g = GlowNode::create(); g->setTag(77); holder->addChild(g); }
    s = CCSprite::create(); s->setTag(78); holder->addChild(s);
    fxLayer->addChild(holder);
    e.node = holder;
    fxs.push_back(e);
    updateEffects(0);
}

void PlayLayer::updateEffects(float dt) {
    CCSpriteFrameCache* fc = CCSpriteFrameCache::sharedSpriteFrameCache();
    for (size_t i = 0; i < fxs.size();) {
        Fx& e = fxs[i];
        e.t += dt;
        float k = std::min(1.f, e.t / e.dur);
        if (e.t >= e.dur + 1e-4f && dt > 0) { e.node->removeFromParent(); fxs.erase(fxs.begin() + i); continue; }
        CCSprite* s = (CCSprite*)e.node->getChildByTag(78);
        char name[64];
        int idx = 1 + std::min(e.n - 1, (int)floorf(k * e.n));
        if (e.n > 1) sprintf(name, "%s_%03d.png", e.base.c_str(), idx); else sprintf(name, "%s_001.png", e.base.c_str());
        if (CCSpriteFrame* fr = fc->spriteFrameByName(name)) s->setDisplayFrame(fr);
        if (e.kind == "splash") s->setScale(1.1f);
        else if (e.kind == "cloud") s->setScale(1.2f);
        else if (e.kind == "flash") {
            s->setScale(1.6f * e.radius); s->setOpacity((GLubyte)(255 * (1 - k)));
            GlowNode* g = (GlowNode*)e.node->getChildByTag(77);
            g->radius = e.radius * cell * (0.6f + 1.2f * k); g->alpha = 1 - k;
        } else if (e.kind == "iceblast") { s->setScale(0.6f + 1.6f * k); s->setOpacity((GLubyte)(255 * (1 - k))); }
        else if (e.kind == "ball") {
            s->setScale(0.8f);
            e.node->setPosition(ccp(e.a.x + (e.b.x - e.a.x) * k, e.a.y + (e.b.y - e.a.y) * k + sinf(k * 3.14159265f) * 30));
        }
        i++;
    }
    for (size_t i = 0; i < popups.size();) {
        Popup& p = popups[i];
        p.t += dt;
        if (p.t < 0) { p.lbl->setVisible(false); i++; continue; }
        float sh = p.t < 0.8f ? 1.f : 1 - powf((p.t - 0.8f) / 0.4f, 2);
        if (p.t >= 1.2f || sh <= 0.01f) { p.lbl->removeFromParent(); popups.erase(popups.begin() + i); continue; }
        float v = p.pts / p.mod, tier = v >= 500 ? 1.3f : v >= 400 ? 1.2f : v >= 300 ? 1.1f : v >= 200 ? 1.0f : v >= 100 ? 0.9f : 0.8f;
        p.lbl->setVisible(true);
        p.lbl->setScale(tier * 1.04f * sh);
        p.lbl->setPosition(ccp(p.pos.x, p.pos.y + 4 + 14.5f * (p.t / 1.2f)));
        i++;
    }
}

// CCParticleSystemQuad from the game's plists; their texture (e.g. starParticle.png) is a frame of particleImgSheet
CCParticleSystemQuad* PlayLayer::spawnParticles(const char* name, CCPoint pos, int z, const float* tint, CCNode* parent) {
    std::string path = CCFileUtils::sharedFileUtils()->fullPathForFilename((std::string(name) + ".plist").c_str());
    CCDictionary* d = CCDictionary::createWithContentsOfFile(path.c_str());
    if (!d) return NULL;
    std::string frame = d->valueForKey("textureFileName")->getCString();
    d->setObject(CCString::create("particleImgSheet.png"), "textureFileName");
    CCParticleSystemQuad* p = new CCParticleSystemQuad();
    if (!p->initWithDictionary(d)) { delete p; return NULL; }
    p->autorelease();
    // atlas frames can be stored rotated, which the particle quad ignores: bake the frame into its own texture
    static std::map<std::string, CCTexture2D*> baked;
    CCTexture2D*& tex = baked[frame];
    if (!tex) {
        CCSprite* s = CCSprite::createWithSpriteFrameName(frame.c_str());
        if (s) {
            CCSize sz = s->getContentSize();
            CCRenderTexture* rt = CCRenderTexture::create((int)ceilf(sz.width), (int)ceilf(sz.height));
            s->setPosition(ccp(sz.width / 2, sz.height / 2));
            s->setFlipY(true);
            rt->begin(); s->visit(); rt->end();
            tex = rt->getSprite()->getTexture(); tex->retain(); tex->setAntiAliasTexParameters();
        }
    }
    if (tex) { CCSize ts = tex->getContentSize(); p->setTexture(tex); p->setTextureWithRect(tex, CCRectMake(0, 0, ts.width, ts.height)); }
    // plist sizes are design points; the quad system treats them as pixels of the content-scaled frame buffer
    float k = 1.0f / CC_CONTENT_SCALE_FACTOR();
    p->setStartSize(p->getStartSize() * k); p->setStartSizeVar(p->getStartSizeVar() * k);
    p->setEndSize(p->getEndSize() * k); p->setEndSizeVar(p->getEndSizeVar() * k);
    p->setPositionType(kCCPositionTypeFree);
    p->setAutoRemoveOnFinish(true);
    p->setPosition(pos);
    if (tint) { // PlayLayer tints with the strong colour but keeps the plist alpha
        ccColor4F s = p->getStartColor(), e = p->getEndColor();
        s.r = e.r = tint[0]; s.g = e.g = tint[1]; s.b = e.b = tint[2];
        p->setStartColor(s); p->setEndColor(e); p->setStartColorVar(ccc4f(0, 0, 0, p->getStartColorVar().a)); p->setEndColorVar(ccc4f(0, 0, 0, p->getEndColorVar().a));
    }
    (parent ? parent : fxLayer)->addChild(p, z);
    return p;
}

// PlayLayer::showScore: one label per cell (points landing on the same cell merge into the older label), delayed until the chain animation ends
void PlayLayer::addScore(int pts, AStarNode* at, int color) {
    if (pts <= 0 || !at) return;
    if (pts >= 40) spawnParticles("starEffect", posOf(at));
    float mod = GameTurnManager::levelModVal(level);
    int total = pts;
    for (Popup& o : popups) if (o.cell == at && o.t < 1.2f) { total += o.pts; o.t = 1.2f; }
    char b[16]; sprintf(b, "%d", total);
    CCLabelBMFont* l = CCLabelBMFont::create(b, "scoreFont2-hd.fnt");
    int hex = (int)strtol(kScoreColors[color >= 1 && color <= 6 ? color : 0][0] ? kScoreColors[color] : "ffffff", NULL, 16);
    l->setColor(ccc3((hex >> 16) & 255, (hex >> 8) & 255, hex & 255));
    fxLayer->addChild(l, 10);
    Popup p; p.lbl = l; p.cell = at; p.pts = total; p.mod = mod; p.t = -0.26f; p.pos = posOf(at);
    l->setVisible(false);
    popups.push_back(p);
}
