// PlayLayer (decompiled PlayLayer.c): the game screen. Board and HUD use the real layout in 320x480 design points
// (grid layer at (0,108), 40pt cells; top bar, points bar, pause button, bottom panel, score/level counters).
#pragma once
#include "cocos2d.h"
#include "AStarGrid.h"
#include "GamePieceManager.h"
#include "GameTurnManager.h"
#include "PauseLayer.h"
#include <set>
#include <map>

class PlayLayer : public cocos2d::CCLayer {
public:
    static cocos2d::CCScene* scene();
    CREATE_FUNC(PlayLayer);
    virtual bool init();
    virtual ~PlayLayer();
    virtual void ccTouchesBegan(cocos2d::CCSet* t, cocos2d::CCEvent* e);
    virtual void update(float dt);
private:
    enum Phase { kIdle, kMoving, kResolving, kLevelOver, kDead, kPow };
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
    // effects (JS drawEffect): frame sequences, glow, ice blast, pipe ball and the floating score labels
    struct Fx { std::string kind; cocos2d::CCNode* node; float t, dur; cocos2d::CCPoint a, b; int radius; std::string base; int n; };
    struct Popup { cocos2d::CCLabelBMFont* lbl; AStarNode* cell; int pts; float mod; float t; cocos2d::CCPoint pos; };
    std::vector<Fx> fxs;
    std::vector<Popup> popups;
    cocos2d::CCNode* fxLayer = NULL;
    void addEffect(const char* kind, AStarNode* at, int color, int radius, AStarNode* to);
    cocos2d::CCParticleSystemQuad* spawnParticles(const char* name, cocos2d::CCPoint pos, int z = 5, const float* tint = NULL, cocos2d::CCNode* parent = NULL);
    cocos2d::CCNode* trailLayer = NULL;
    GamePiece* trailPiece = NULL;
    cocos2d::CCParticleSystemQuad* trail = NULL;
    struct Warn { cocos2d::CCParticleSystemQuad* ps; int tick; };
    std::map<GamePiece*, Warn> warns;
    void updateWarns();
    void warmUp();
    void addScore(int pts, AStarNode* at, int color);
    void updateEffects(float dt);
    // pause + powerup slots
    bool paused = false;
    float playTime = 0, powTimer = 0;
    PauseLayer* pauseLayer = NULL;
    cocos2d::CCLabelBMFont* frozenLbl = NULL;
    struct Slot { std::string tag; int rank; int charges; cocos2d::CCSprite *icon, *badge; cocos2d::CCRect r; };
    std::vector<Slot> slots;
    void setupSlots();
    void updateSlots();
    bool canUsePower(const Slot& s) const;
    void usePower(Slot& s);
    void pauseGame();
    void setTreePaused(cocos2d::CCNode* n, bool p);
    void restart();
    void completeLevel();
};
