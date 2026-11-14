#include "GamePieceManager.h"
#include <cmath>
#include <algorithm>
USING_NS_CC;

static const int kSpecials[6] = { 7, 5, 6, 11, 9, 8 };

PieceColor GamePieceManager::randomPieceColor(int level) {
    static const int table[5] = { 1, 4, 3, 2, 5 };
    int n = level < 5 ? level + 2 : 6;
    int k = (int)ceilf(CCRANDOM_0_1() * n);
    return (PieceColor)(k >= 1 && k <= 5 ? table[k - 1] : 6);
}
int GamePieceManager::maxCountForAttr(int a) const {
    if (a == kAttrBomb) return 10;
    if (a == kAttrColor) return 4;
    int cap;
    switch (a) { case 5: case 6: cap = 6; break; case 7: cap = 4; break; case 8: cap = 10; break; case 9: cap = 1; break; case 11: cap = 6; break; default: return 0; }
    int l = level();
    return l > 0 ? std::min(l, cap) : 0;
}
float GamePieceManager::maxChanceForAttr(int a) const {
    float start, base, grow;
    switch (a) {
        case 5: start = 3; base = 4.0f; grow = 0.6f; break;
        case 6: start = 5; base = 3.0f; grow = 0.5f; break;
        case 7: start = 7; base = 2.0f; grow = 0.6f; break;
        case 8: start = 11; base = 4.0f; grow = 0.8f; break;
        case 9: start = 9; base = 0.4f; grow = 0.4f; break;
        case 11: start = 13; base = 5.0f; grow = 0.8f; break;
        default: return 0;
    }
    int l = level();
    return l >= start ? base + (l - start) * grow : 0;
}
float GamePieceManager::modForAttribute(int a) const {
    float f;
    if (a == 5 || a == 8 || a == 11) f = 0.2f; else if (a == 7 || a == 9) f = 0.1f; else if (a == 6) f = 0.15f; else return 0;
    return countOf(a) < maxCountForAttr(a) ? maxChanceForAttr(a) * f : 0;
}
void GamePieceManager::resetChances() { for (int a : kSpecials) m_c[a] = maxChanceForAttr(a) * 0.5f; }
void GamePieceManager::incrementChances() {
    for (int a : kSpecials) m_c[a] += modForAttribute(a);
    if (!countOf(kAttrMegaBomb)) m_megaBomb += (rank("kPowBombM") >= 3 ? 1.5f : 1.f) * 0.0015f;
    if (!countOf(kAttrMegaColor)) m_megaColor += (rank("kPowColorM") >= 3 ? 1.5f : 1.f) * 0.001f;
}
void GamePieceManager::spawnedPiece(int a) {
    if (a == 9) { m_c[9] = -4.0f; return; }
    if (!m_c.count(a)) return;
    int mx = maxCountForAttr(a);
    float ratio = mx == 0 ? 0 : (float)(mx - countOf(a)) / mx;
    m_c[a] *= ratio;
}
float GamePieceManager::pieceAttrChance(int a) const {
    if (isAtMaxCount(a) && a != kAttrColor) return 0;
    float v, lv = (float)level();
    if (a == kAttrBomb) {
        float k = floorf(std::max(0, 2 - countOf(1)) * 1.5f + 1);
        v = std::min((lv * 0.5f + 8) * k, k * 12);
        if (rank("kPowBombM") > 0) v *= 1.1f;
    } else if (a == kAttrColor) {
        int cnt = countOf(3);
        float k = floorf(std::max(0, 1 - cnt) * 1.5f + 1);
        v = std::min((lv * 0.5f + 8) * k, k * 12);
        if (rank("kPowColorM") > 0) v *= 1.1f;
        if (cnt >= 6) v *= 0.1f; else if (cnt >= 4) v *= 0.2f;
    } else {
        auto it = m_c.find(a);
        v = it == m_c.end() ? 0 : it->second;
        if (a == kAttrMysteryBox && rank("kPowGreed") >= 3) v *= 1.15f;
    }
    return std::max(0.f, v);
}
PieceAttribute GamePieceManager::getRandomPieceAttribute() const {
    float r = CCRANDOM_0_1() * 100, acc = 0;
    static const int order[8] = { 3, 1, 8, 5, 6, 7, 9, 11 };
    for (int a : order) { acc += pieceAttrChance(a); if (r < acc) return (PieceAttribute)a; }
    return kAttrBlock;
}
PieceAttribute GamePieceManager::upgrade(PieceAttribute a) {
    if (a == kAttrBomb && !countOf(kAttrMegaBomb) && rank("kPowBombM") > 1 && CCRANDOM_0_1() <= m_megaBomb) { m_megaBomb = 0; return kAttrMegaBomb; }
    if (a == kAttrColor && !countOf(kAttrMegaColor) && rank("kPowColorM") > 1 && CCRANDOM_0_1() <= m_megaColor) { m_megaColor = 0; return kAttrMegaColor; }
    return a;
}
GamePiece* GamePieceManager::createRandomPiece() {
    PieceAttribute a = upgrade(getRandomPieceAttribute());
    spawnedPiece(a);
    return GamePiece::create(randomPieceColor(level()), a);
}
