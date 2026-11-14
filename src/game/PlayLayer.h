// PlayLayer (decompiled PlayLayer.c): the game screen. Board and HUD use the real layout in 320x480 design points
// (grid layer at (0,108), 40pt cells; top bar, points bar, pause button, bottom panel, score/level counters).
#pragma once
#include "cocos2d.h"
#include "AStarGrid.h"
#include "GamePieceManager.h"
#include "GameTurnManager.h"
#include <set>

class PlayLayer : public cocos2d::CCLayer {
public:
    static cocos2d::CCScene* scene();
    CREATE_FUNC(PlayLayer);
    virtual bool init();
    virtual ~PlayLayer();
    virtual void ccTouchesBegan(cocos2d::CCSet* t, cocos2d::CCEvent* e);
    virtual void update(float dt);
private:
    enum Phase { kIdle, kMoving, kResolving, kLevelOver, kDead };
    static const int N = 8;
    float cell = 40, ox = 0, oy = 107.4f;
    AStarGrid* grid = NULL;
    GamePieceManager mgr;
    GameTurnManager* turn = NULL;
    cocos2d::CCLayer* boardLayer = NULL;
    cocos2d::CCNode* hintLayer = NULL;
    cocos2d::CCSprite *barFill = NULL, *selectSprite = NULL;
    cocos2d::CCLabelBMFont *scoreLbl = NULL, *levelLbl = NULL, *bigLbl = NULL;
    AStarNode *selected = NULL, *landing = NULL;
    Phase phase = kIdle;
    int level = 0, score = 0, levelPoints = 0, target = 1;
    float levelTimer = 0;
    cocos2d::CCPoint posOf(AStarNode* n) const { return ccp(ox + (n->gx + 0.5f) * cell, oy + (n->gy + 0.5f) * cell); }
    int countOf(int attr) const;
    GamePiece* spawnAt(AStarNode* n, bool animate);
    void spawnRandom(int n);
    void startLevel();
    void layoutHud();
    void updateHud();
    void showBig(const char* text, float dur);
    void setSelected(AStarNode* n);
    void movePiece(AStarNode* from, AStarNode* to);
    void landed(cocos2d::CCNode*);
    void endTurn();
    static int levelTarget(int level);
};
