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
        for (AStarNode* c : m_grid->neighborsToNode(node, mega ? 2 : 1)) {
            GamePiece* q = c->piece;
            if (!q || q == p || q->pieceAttribute() == kAttrStone || q->pieceColor() == p->pieceColor()) continue;
            q->changePiece(p->pieceColor(), q->pieceAttribute());
        }
        after(0.45f, [=] { m_pending--; });
    });
}

void GameTurnManager::startChain(const std::vector<AStarNode*>& group, AStarNode* origin) {
    int chainId = ++m_chainCount;
    if (onChain) onChain(chainId);
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
    after(0.07f * i + 0.12f, [=] { givePoints(basePointsForLen(len), chainId, ordered.back()); m_pending--; });
}

void GameTurnManager::destroyPiece(AStarNode* n, int chainId, int idx) {
    GamePiece* p = n->piece;
    if (!p || m_dying.count(p)) return;
    m_dying.insert(p);
    n->piece = NULL;
    PieceAttribute a = p->pieceAttribute();
    auto finish = [=] { m_dying.erase(p); p->removeFromParent(); };
    p->runAnimation("Charge", false, [=] { if (!p->runAnimation("Explode", false, finish)) finish(); });
    if (a == kAttrBomb || a == kAttrMegaBomb) {
        m_pending++;
        after(0.38f, [=] { explodeBomb(p, n, chainId); m_pending--; });
    }
}

// GameTurnManager::checkNewChainFromNode: a bomb clears its surroundings; groups of 4+ in the blast start their own chain
void GameTurnManager::explodeBomb(GamePiece* bomb, AStarNode* cell, int chainId) {
    int count = 1; // the bomb itself
    for (AStarNode* c : m_grid->neighborsToNode(cell, bomb->pieceAttribute() == kAttrMegaBomb ? 2 : 1)) {
        GamePiece* q = c->piece;
        if (!q || m_dying.count(q) || c == cell) continue;
        if (q->pieceAttribute() == kAttrStone) continue;
        if (m_active.count(q)) { count++; continue; }
        std::vector<AStarNode*> g = m_grid->sameTypeConnected(c);
        if (g.size() >= 4) { startChain(g, c); continue; }
        m_chainId[q] = chainId;
        destroyPiece(c, chainId, count++);
    }
    givePoints((basePointsForLen(count + 1)) / 2, chainId, cell);
}

void GameTurnManager::givePoints(int base, int chainId, AStarNode* at) {
    int pts = (int)floorf(base * multiplierForChain(chainId) * levelModVal(level()));
    if (onPoints) onPoints(pts, at);
}
