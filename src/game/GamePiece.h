// GamePiece (decompiled GamePiece.c): one boomling on the grid. Colour/attribute enums follow the game's values.
#pragma once
#include "CCAnimatedSprite.h"

enum PieceColor { kColorGreen = 1, kColorRed = 2, kColorOrange = 3, kColorBlue = 4, kColorPurple = 5, kColorYellow = 6 };
enum PieceAttribute { kAttrBlock = 0, kAttrBomb = 1, kAttrMegaBomb = 2, kAttrColor = 3, kAttrMegaColor = 4, kAttrIceBomb = 5, kAttrPipe = 6, kAttrSkull = 7, kAttrStone = 8, kAttrMysteryBox = 9, kAttrChanger = 11 };

class GamePiece : public CCAnimatedSprite {
public:
    static GamePiece* create(PieceColor color, PieceAttribute attr);
    bool init(PieceColor color, PieceAttribute attr);
    static const char* colorToString(int color);
    static const char* attributeToString(int attr);
    static std::string getTypeForColor(int color, int attr);
    // GamePiece::changePiece: new colour/attribute, reload the animated type (keeps the current animation)
    void changePiece(PieceColor color, PieceAttribute attr);
    // GamePiece::getStrongColorValue (colour of the move trail)
    cocos2d::ccColor3B getStrongColorValue() const;
    PieceColor pieceColor() const { return m_color; }
    PieceAttribute pieceAttribute() const { return m_attr; }
    bool turnSensitive() const { return m_turnSensitive; }
    // turn counter of tickers (Skull 8, MysteryBox 4, IceBomb 4), stone health, ice overlay / immovable flags
    int tick = 0, health = 2;
    bool frozen = false, iced = false, locked = false;
    bool isTicker() const { return m_attr == kAttrSkull || m_attr == kAttrIceBomb || m_attr == kAttrMysteryBox; }
    virtual void animationFinished(const std::string& name);
    // PlayLayer sets the layer that holds the blob shadows (drawn under every piece)
    static cocos2d::CCNode* shadowLayer;
    virtual void update(float dt);
    virtual void onExit();
protected:
    PieceColor m_color = kColorGreen;
    PieceAttribute m_attr = kAttrBlock;
    bool m_turnSensitive = false;
    float m_idleTimer = 4;
    cocos2d::CCSprite* m_shadow = NULL;
    cocos2d::CCSprite* m_count = NULL;
};
