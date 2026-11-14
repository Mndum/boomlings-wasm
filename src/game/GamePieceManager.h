// GamePieceManager (decompiled GamePieceManager.c): what the next spawned piece is. Same model as js/rules.js Chances.
#pragma once
#include "GamePiece.h"
#include <functional>
#include <map>

class GamePieceManager {
public:
    std::function<int()> level = [] { return 1; };                 // current level
    std::function<int(int)> countOf = [](int) { return 0; };       // live pieces per attribute
    std::function<int(const char*)> rank = [](const char*) { return 0; }; // powerup rank (kPowBombM, kPowColorM, kPowGreed)
    GamePieceManager() { resetChances(); }
    static PieceColor randomPieceColor(int level);
    int maxCountForAttr(int a) const;
    bool isAtMaxCount(int a) const { return countOf(a) >= maxCountForAttr(a); }
    float maxChanceForAttr(int a) const;
    float modForAttribute(int a) const;
    float pieceAttrChance(int a) const;
    void resetChances();
    void incrementChances();
    void spawnedPiece(int a);
    PieceAttribute getRandomPieceAttribute() const;
    PieceAttribute upgrade(PieceAttribute a);
    GamePiece* createRandomPiece();
private:
    std::map<int, float> m_c;
    float m_megaBomb = 0, m_megaColor = 0;
};
