// GameTurnManager (decompiled GameTurnManager.c): resolves one turn after a piece landed - Colour blobs recolour their
// neighbours, groups of 4+ same-coloured pieces form a chain and are destroyed one after another, Bombs blow up their
// surroundings (and can start further chains) and every chain/blast scores points. Specials (skull, ice bomb, mystery box,
// stone) are not ported yet. Timings follow the JS port that was measured against the real game.
#pragma once
#include "AStarGrid.h"
#include <functional>
#include <map>
#include <set>

class GameTurnManager {
public:
    explicit GameTurnManager(AStarGrid* grid) : m_grid(grid) {}
    std::function<int()> level = [] { return 1; };
    std::function<void(int points, AStarNode* at)> onPoints;
    std::function<void(int chainId)> onChain;
    // scoring tables (PlayLayer / GameManager)
    static int basePointsForLen(int n) { return ((n + 1) * n) & ~1; }
    static int multiplierForChain(int c) { static const int t[7] = { 1, 1, 2, 4, 8, 12, 16 }; return c + 1 < 8 ? t[c] : 20; }
    static float levelModVal(int level) { return std::max(1.0f, 1 + (level - 1) * 0.1f); }
    // call after a piece landed on `node`
    void pieceLanded(AStarNode* node);
    void tick(float dt);
    bool busy() const { return m_pending > 0 || !m_timers.empty(); }
    int chainCount() const { return m_chainCount; }
private:
    struct Timer { float t; std::function<void()> fn; };
    AStarGrid* m_grid;
    std::vector<Timer> m_timers;
    std::set<GamePiece*> m_active, m_dying;
    std::map<GamePiece*, int> m_chainId;
    int m_chainCount = 0, m_pending = 0;
    void after(float t, std::function<void()> fn) { m_timers.push_back({ t, fn }); }
    void activateColor(AStarNode* node);
    void startChain(const std::vector<AStarNode*>& group, AStarNode* origin);
    void destroyPiece(AStarNode* n, int chainId, int idx);
    void explodeBomb(GamePiece* bomb, AStarNode* cell, int chainId);
    void givePoints(int base, int chainId, AStarNode* at);
};
