#include "GameTurnManager.h"
#include <algorithm>
#include <deque>
USING_NS_CC;

void GameTurnManager::tick(float dt) {
    std::vector<Timer> due, keep;
    for (Timer& t : m_timers) { t.t -= dt; (t.t <= 0 ? due : keep).push_back(t); }
    m_timers = keep;
    for (Timer& t : due) t.fn();
    if (m_timers.empty() && m_pending == 0) { m_chainCount = 0; m_active.clear(); m_chainId.clear(); }
}

void GameTurnManager::pieceLanded(AStarNode* node) {
    GamePiece* p = node->piece;
    if (!p) return;
    PieceAttribute a = p->pieceAttribute();
    if (a == kAttrBomb || a == kAttrMegaBomb) return;
    if (a == kAttrColor || a == kAttrMegaColor) { activateColor(node); return; }
    std::vector<AStarNode*> group = m_grid->sameTypeConnected(node);
    if (group.size() >= 4) startChain(group, node);
}

// PlayLayer::activateColorAt: Charge animation, then the blob becomes a Block and recolours its neighbours
void GameTurnManager::activateColor(AStarNode* node) {
    GamePiece* p = node->piece;
    bool mega = p->pieceAttribute() == kAttrMegaColor;
    p->runAnimation("Charge", false);
    m_pending++;
    after(0.3f, [=] {
        p->changePiece(p->pieceColor(), kAttrBlock);
        p->runAnimation("Idle01", true);
        snd(mega ? "megaSplat001.ogg" : "splat03.ogg", 0.8f);
        for (AStarNode* c : m_grid->neighborsToNode(node, mega ? 2 : 1)) {
            GamePiece* q = c->piece;
            if (!q || q == p || q->pieceAttribute() == kAttrStone || q->pieceColor() == p->pieceColor()) continue;
            q->changePiece(p->pieceColor(), q->pieceAttribute());
            fx("splash", c, p->pieceColor());
        }
        fx("splash", node, p->pieceColor());
        after(0.45f, [=] { m_pending--; });
    });
}

void GameTurnManager::startChain(const std::vector<AStarNode*>& group, AStarNode* origin) {
    int chainId = ++m_chainCount;
    int chainColor = origin->piece ? origin->piece->pieceColor() : 0;
    if (onChain) onChain(chainId);
    if (chainId > 1) snd("chainTest01.ogg", 0.8f);
    if (group.size() >= 10) snd("whoa.ogg", 0.9f);
    for (AStarNode* c : group) { m_chainId[c->piece] = chainId; m_active.insert(c->piece); }
    // destroy nearest to the origin first
    std::map<AStarNode*, int> dist;
    std::deque<AStarNode*> q;
    dist[origin] = 0; q.push_back(origin);
    while (!q.empty()) {
        AStarNode* c = q.front(); q.pop_front();
        for (AStarNode* n : c->neighbors)
            if (std::find(group.begin(), group.end(), n) != group.end() && !dist.count(n)) { dist[n] = dist[c] + 1; q.push_back(n); }
    }
    std::vector<AStarNode*> ordered = group;
    std::stable_sort(ordered.begin(), ordered.end(), [&](AStarNode* a, AStarNode* b) { return dist[a] < dist[b]; });
    m_pending++;
    int i = 0;
    for (AStarNode* c : ordered) {
        int idx = i++;
        after(0.07f * idx, [=] { if (c->piece && m_chainId[c->piece] == chainId) destroyPiece(c, chainId, idx); });
    }
    int len = (int)group.size();
    after(0.07f * i + 0.12f, [=] { givePoints(basePointsForLen(len), chainId, ordered.back(), chainColor); m_pending--; if (m_pending == 0) handleSpecials(); });
}

void GameTurnManager::destroyPiece(AStarNode* n, int chainId, int idx) {
    GamePiece* p = n->piece;
    if (!p || m_dying.count(p)) return;
    PieceAttribute a = p->pieceAttribute();
    if (a == kAttrMysteryBox && chainId) {
        p->locked = true; m_boxes.push_back(n);
        p->runAnimation("Charge", false, [=] { p->runAnimation("Idle01", true); });
        return;
    }
    if (a == kAttrStone && p->health > 0 && !chainId) { damageStone(n, 1); return; }
    m_dying.insert(p);
    n->piece = NULL;
    snd(idx % 2 ? "pop2b2.ogg" : "pops2a.ogg", 0.6f);
    if (a == kAttrBomb || a == kAttrMegaBomb) snd(a == kAttrMegaBomb ? "megaExplode001.ogg" : "bombbbb.ogg", 0.9f);
    if (a == kAttrStone) snd("stoneHit2.ogg", 0.8f);
    if (a == kAttrBomb || a == kAttrMegaBomb || a == kAttrMysteryBox) fx("charge", n, 0, a == kAttrMegaBomb ? 2 : 1);
    auto finish = [=] { m_dying.erase(p); p->removeFromParent(); };
    bool hasExplode = AnimLibrary::shared()->get(p->type(), "Explode") != NULL;
    if (!hasExplode) fx("cloud", n);
    p->runAnimation("Charge", false, [=] { if (hasExplode) p->runAnimation("Explode", false, finish); else finish(); });
    if (a == kAttrBomb || a == kAttrMegaBomb) {
        m_pending++;
        after(0.38f, [=] { explodeBomb(p, n, chainId); m_pending--; });
    }
}

// GameTurnManager::checkNewChainFromNode: a bomb clears its surroundings; groups of 4+ in the blast start their own chain
void GameTurnManager::explodeBomb(GamePiece* bomb, AStarNode* cell, int chainId) {
    fx("flash", cell, bomb->pieceColor() ? bomb->pieceColor() : 6, bomb->pieceAttribute() == kAttrMegaBomb ? 2 : 1);
    int count = 1; // the bomb itself
    for (AStarNode* c : m_grid->neighborsToNode(cell, bomb->pieceAttribute() == kAttrMegaBomb ? 2 : 1)) {
        GamePiece* q = c->piece;
        if (!q || m_dying.count(q) || c == cell) continue;
        if (q->pieceAttribute() == kAttrStone) { damageStone(c, 1); continue; }
        if (m_active.count(q)) { count++; continue; }
        std::vector<AStarNode*> g = m_grid->sameTypeConnected(c);
        if (g.size() >= 4) { startChain(g, c); continue; }
        m_chainId[q] = chainId;
        destroyPiece(c, chainId, count++);
    }
    givePoints((basePointsForLen(count + 1)) / 2, chainId, cell, bomb->pieceColor());
}

void GameTurnManager::givePoints(int base, int chainId, AStarNode* at, int color) {
    int pts = (int)floorf(base * multiplierForChain(chainId) * levelModVal(level()));
    if (onPoints) onPoints(pts, at, color);
}

void GameTurnManager::damageStone(AStarNode* n, int dmg) {
    GamePiece* s = n->piece;
    s->health -= dmg;
    if (s->health <= 0) { s->health = 0; destroyPiece(n, 1, 0); return; }
    s->runAnimation("Idle02", true);
    fx("stonehit", n);
    snd("stoneHit1_m.ogg", 0.8f);
}

void GameTurnManager::removeNow(AStarNode* n) {
    GamePiece* p = n->piece;
    if (!p) return;
    n->piece = NULL;
    p->removeFromParent();
}

// GameTurnManager::handleSpecialNodes + GameManager::mysteryBoxTriggered
void GameTurnManager::handleSpecials() {
    std::vector<AStarNode*> boxes;
    boxes.swap(m_boxes);
    float t = 0;
    for (AStarNode* b : boxes) {
        if (!b->piece) continue;
        m_pending++;
        t += 0.5f;
        after(t, [=] {
            snd("mysteryTrigger001.ogg", 0.9f); snd("spinner.ogg", 0.6f);
            int reward = rand() % 5, skulls = 0;
            for (int x = 0; x < m_grid->width(); x++) for (int y = 0; y < m_grid->height(); y++) {
                AStarNode* n = m_grid->nodeAt(x, y);
                if (n->piece && n->piece->pieceAttribute() == kAttrSkull) skulls++;
            }
            if (reward == 4 && skulls < 1) reward = 3;
            if (reward <= 2) bonusGold += 10 * (reward + 1);
            else if (reward == 3) timeFrozen = timeFrozen < 1 ? 2 : timeFrozen + 2;
            else {
                for (int x = 0; x < m_grid->width(); x++) for (int y = 0; y < m_grid->height(); y++) {
                    AStarNode* n = m_grid->nodeAt(x, y);
                    if (n->piece && n->piece->pieceAttribute() == kAttrSkull) destroyPiece(n, 1, 0);
                }
            }
            { static const char* rw[3] = { "mysteryReward001.ogg", "mysteryReward002.ogg", "mysteryReward003.ogg" }; snd(rw[rand() % 3], 0.9f); }
            if (b->piece) { fx("cloud", b); removeNow(b); }
            after(0.6f, [=] { m_pending--; });
        });
    }
}

void GameTurnManager::updateTurnSensitive() {
    std::vector<AStarNode*> sens, timedOut;
    for (int x = 0; x < m_grid->width(); x++) for (int y = 0; y < m_grid->height(); y++) {
        AStarNode* n = m_grid->nodeAt(x, y);
        if (n->piece && n->piece->turnSensitive()) sens.push_back(n);
    }
    for (AStarNode* n : sens) {
        GamePiece* p = n->piece;
        if (!p) continue;
        if (p->isTicker()) {
            p->tick = std::max(0, p->tick - 1);
            if (p->tick == 0) timedOut.push_back(n);
            else if (p->pieceAttribute() == kAttrSkull && p->tick <= 3) snd("alert04.ogg", 0.6f);
        } else if (p->pieceAttribute() == kAttrPipe) {
            std::vector<AStarNode*> free;
            for (AStarNode* o : n->neighbors) if (!o->piece) free.push_back(o);
            if (!free.empty() && spawnAt) {
                AStarNode* dst = free[rand() % free.size()];
                GamePiece* np = spawnAt(dst);
                fx("ball", n, np ? np->pieceColor() : 1, 1, dst);
                p->runAnimation("ExitAnim", false, [=] { p->runAnimation("Idle01", true); });
            }
        } else if (p->pieceAttribute() == kAttrChanger) {
            int c = p->pieceColor();
            for (int i = 0; i < 10 && c == p->pieceColor(); i++) c = 1 + rand() % 6;
            p->changePiece((PieceColor)c, kAttrChanger);
            snd("splatSound.ogg", 0.5f);
        }
    }
    for (AStarNode* n : timedOut) {
        GamePiece* p = n->piece;
        if (!p) continue;
        switch (p->pieceAttribute()) {
            case kAttrIceBomb:
                for (AStarNode* o : n->neighbors)
                    if (o->piece && o->piece->pieceAttribute() != kAttrStone && o->piece->pieceAttribute() != kAttrPipe) { o->piece->frozen = true; o->piece->iced = true; }
                snd("frostBoom2.ogg", 0.9f);
                fx("iceblast", n); fx("cloud", n);
                removeNow(n);
                break;
            case kAttrSkull: skullDeath = true; break;
            case kAttrMysteryBox: fx("cloud", n); removeNow(n); break;
            default: break;
        }
    }
}
