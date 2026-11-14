#include "GamePiece.h"
USING_NS_CC;

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
    m_idleTimer = 2 + 5 * CCRANDOM_0_1();
    runAnimation("Idle01", false);
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
