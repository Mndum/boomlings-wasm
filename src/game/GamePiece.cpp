#include "GamePiece.h"
USING_NS_CC;
CCNode* GamePiece::shadowLayer = NULL;

static bool isTurnSensitive(int attr) {
    return attr == kAttrSkull || attr == kAttrIceBomb || attr == kAttrPipe || attr == kAttrMysteryBox || attr == kAttrChanger;
}
const char* GamePiece::colorToString(int c) {
    switch (c) { case 2: return "r"; case 3: return "o"; case 4: return "b"; case 5: return "p"; case 6: return "y"; default: return "g"; }
}
const char* GamePiece::attributeToString(int a) {
    switch (a) {
        case 1: return "Bomb"; case 2: return "MegaBomb"; case 3: return "Color"; case 4: return "MegaColor"; case 5: return "IceBomb";
        case 6: return "Pipe"; case 7: return "Skull"; case 9: return "MysteryBox"; case 11: return "Changer"; default: return "Block";
    }
}
std::string GamePiece::getTypeForColor(int color, int attr) {
    if (attr == kAttrStone) return "Stone";
    return std::string(attributeToString(attr)) + "_" + colorToString(color);
}
GamePiece* GamePiece::create(PieceColor color, PieceAttribute attr) {
    GamePiece* p = new GamePiece();
    if (p && p->init(color, attr)) { p->autorelease(); return p; }
    delete p;
    return NULL;
}
bool GamePiece::init(PieceColor color, PieceAttribute attr) {
    if (!initWithType(getTypeForColor(color, attr).c_str())) return false;
    m_color = color; m_attr = attr;
    m_turnSensitive = isTurnSensitive(attr);
    tick = attr == kAttrSkull ? 8 : (attr == kAttrIceBomb || attr == kAttrMysteryBox) ? 4 : 0;
    frozen = attr == kAttrStone || attr == kAttrPipe;
    m_idleTimer = 2 + 5 * CCRANDOM_0_1();
    m_holder->setPosition(ccp(0.165f, 1.2f)); // PlayLayer draws the animated sprite at (+0.33, -2.4) px from the cell centre
    runAnimation("Idle01", false);
    if (shadowLayer) {
        m_shadow = CCSprite::createWithSpriteFrameName("TileShadow01_001.png");
        m_shadow->setOpacity(56); shadowLayer->addChild(m_shadow);
    }
    return true;
}
void GamePiece::changePiece(PieceColor color, PieceAttribute attr) {
    m_color = color; m_attr = attr;
    m_turnSensitive = isTurnSensitive(attr);
    loadType(getTypeForColor(color, attr).c_str());
}
ccColor3B GamePiece::getStrongColorValue() const {
    switch (m_color) {
        case 1: return ccc3(0x64, 0xff, 0x50);
        case 2: return ccc3(0xff, 0x6e, 0x96);
        case 3: return ccc3(0xff, 0xa5, 0x4b);
        case 4: return ccc3(0x64, 0xa5, 0xff);
        case 5: return ccc3(0xff, 0x6e, 0xfa);
        case 6: return ccc3(0xff, 0xff, 0x64);
        default: return ccc3(0xff, 0xff, 0xff);
    }
}
// an idle animation that ended restarts Idle01; after a random pause the piece may play Idle02 or Idle03 instead
void GamePiece::animationFinished(const std::string& name) {
    if (name.compare(0, 4, "Idle") == 0) {
        m_idleTimer -= 0.0833f * 4;
        if (m_idleTimer <= 0 && m_attr != kAttrChanger) {
            m_idleTimer = 2 + 5 * CCRANDOM_0_1();
            runAnimation(CCRANDOM_0_1() < 0.5f ? "Idle02" : "Idle03", false);
        } else runAnimation("Idle01", false);
    }
}

// the shadow follows the piece (JS: TileShadow01 at +8px below, scale 1.15, alpha .22); tickers show their turn counter
void GamePiece::startEnter() { m_enterT = 0; setScale(0.1f); setPartOpacity(0); }
void GamePiece::update(float dt) {
    CCAnimatedSprite::update(dt);
    if (m_enterT >= 0) {
        m_enterT += dt;
        float k = std::min(1.f, m_enterT / 0.3f);
        setScale(0.1f + 0.9f * k); setPartOpacity((GLubyte)(255 * k));
        if (k >= 1) m_enterT = -1;
    }
    if (m_shadow) {
        m_shadow->setVisible(getParent() != NULL && m_name != "Explode");
        m_shadow->setPosition(ccp(getPositionX(), getPositionY() - 4));
        m_shadow->setScale(1.15f * getScale());
        m_shadow->setOpacity((GLubyte)(56 * m_partOpacity / 255.f));
    }
    if (isTicker()) {
        char n[32]; sprintf(n, "count_num_%d.png", std::min(12, tick));
        CCSpriteFrame* fr = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(n);
        if (!m_count) { m_count = CCSprite::create(); addChild(m_count, 50); m_count->setPosition(ccp(12, 13)); }
        if (fr) m_count->setDisplayFrame(fr);
        m_count->setVisible(m_name != "Explode");
    }
}
void GamePiece::onExit() {
    if (m_shadow) { m_shadow->removeFromParent(); m_shadow = NULL; }
    CCAnimatedSprite::onExit();
}
