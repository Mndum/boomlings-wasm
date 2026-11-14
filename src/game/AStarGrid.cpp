#include "AStarGrid.h"
#include <algorithm>
#include <map>
#include <set>
USING_NS_CC;

bool AStarNode::has(const std::vector<AStarNode*>& v) const { return std::find(v.begin(), v.end(), this) != v.end(); }

void AStarNode::addNeighborsOfSameType(std::vector<AStarNode*>& out, bool recursive) {
    if (!piece) return;
    for (AStarNode* n : neighbors) {
        if (!n->piece || n->piece->pieceColor() != piece->pieceColor()) continue;
        if (n->has(out)) continue;
        out.push_back(n);
        if (recursive) n->addNeighborsOfSameType(out, true);
    }
}
void AStarNode::addSpecial(std::vector<AStarNode*>& out) {
    if (!piece || piece->pieceAttribute() != kAttrBomb) return;
    for (AStarNode* n : looseNeighbors)
        if (n->piece && !n->has(out)) out.push_back(n);
}

AStarGrid::AStarGrid(int w, int h) : m_w(w), m_h(h) {
    for (int x = 0; x < w; x++) for (int y = 0; y < h; y++) {
        AStarNode* n = new AStarNode();
        n->gx = x; n->gy = y;
        m_nodes.push_back(n);
    }
    for (AStarNode* n : m_nodes) {
        const int d4[4][2] = { {-1,0},{0,-1},{0,1},{1,0} };
        for (auto& d : d4) if (AStarNode* o = nodeAt(n->gx + d[0], n->gy + d[1])) n->neighbors.push_back(o);
        for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) {
            if (!dx && !dy) continue;
            if (AStarNode* o = nodeAt(n->gx + dx, n->gy + dy)) n->looseNeighbors.push_back(o);
        }
    }
}
AStarGrid::~AStarGrid() { for (AStarNode* n : m_nodes) n->release(); }

AStarNode* AStarGrid::nodeAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return NULL;
    return m_nodes[x * m_h + y];
}
AStarNode* AStarGrid::nodeAtPoint(float x, float y, float cell, float ox, float oy) const {
    return nodeAt((int)floorf((x - ox) / cell), (int)floorf((y - oy) / cell));
}

bool AStarGrid::findPath(AStarNode* from, AStarNode* to, std::vector<AStarNode*>& path) const {
    path.clear();
    if (!from || !to || from == to) return false;
    std::map<AStarNode*, float> g;
    std::map<AStarNode*, AStarNode*> prev;
    std::multimap<float, AStarNode*> open;
    std::set<AStarNode*> closed;
    g[from] = 0;
    open.insert({ from->costToNode(to), from });
    while (!open.empty()) {
        AStarNode* cur = open.begin()->second;
        open.erase(open.begin());
        if (closed.count(cur)) continue;
        closed.insert(cur);
        if (cur == to) {
            for (AStarNode* n = to; n != from; n = prev[n]) path.push_back(n);
            path.push_back(from);
            std::reverse(path.begin(), path.end());
            return true;
        }
        for (AStarNode* n : cur->neighbors) {
            if (closed.count(n) || (n->piece && n != to) || !n->active) continue;
            float ng = g[cur] + cur->costToNode(n);
            if (!g.count(n) || ng < g[n]) {
                g[n] = ng; prev[n] = cur;
                open.insert({ ng + n->costToNode(to), n });
            }
        }
    }
    return false;
}

std::vector<AStarNode*> AStarGrid::sameTypeConnected(AStarNode* n) const {
    std::vector<AStarNode*> out;
    if (!n || !n->piece) return out;
    out.push_back(n);
    n->addNeighborsOfSameType(out, true);
    return out;
}

std::vector<AStarNode*> AStarGrid::neighborsToNode(AStarNode* n, int radius) const {
    std::vector<AStarNode*> out;
    out.push_back(n);
    for (int r = 0; r < radius; r++) {
        std::vector<AStarNode*> add;
        for (AStarNode* c : out) for (AStarNode* o : c->neighbors) if (!o->has(out) && !o->has(add)) add.push_back(o);
        out.insert(out.end(), add.begin(), add.end());
    }
    return out;
}

bool AStarGrid::validMoveExists() const {
    for (AStarNode* n : m_nodes) {
        if (!n->piece) continue;
        for (AStarNode* o : n->neighbors) if (!o->piece) return true;
    }
    return false;
}
