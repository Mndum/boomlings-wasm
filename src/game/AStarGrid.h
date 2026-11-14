// AStarNode / AStarGrid (decompiled AStarNode.c, AStarGrid.c): the 8x8 board. Every cell is a node with
// 4 neighbours (addNeighbor) and 8 loose neighbours (addLooseNeighbor); nodes hold the piece standing on them.
#pragma once
#include "cocos2d.h"
#include "GamePiece.h"
#include <vector>

class AStarNode : public cocos2d::CCObject {
public:
    int gx = 0, gy = 0;
    GamePiece* piece = NULL;
    bool active = true;
    float costMultiplier = 1;
    std::vector<AStarNode*> neighbors;      // 4-direction
    std::vector<AStarNode*> looseNeighbors; // 8-direction
    // AStarNode::costToNode: Manhattan distance times costMultiplier
    float costToNode(const AStarNode* o) const { return (fabsf((float)(gx - o->gx)) + fabsf((float)(gy - o->gy))) * costMultiplier; }
    // AStarNode::addNeighborsOfSameTypeToArray: same non-zero colour, not already listed, optionally recursive
    void addNeighborsOfSameType(std::vector<AStarNode*>& out, bool recursive);
    // AStarNode::addSpecialToArray: a Bomb adds every occupied surrounding node
    void addSpecial(std::vector<AStarNode*>& out);
    bool has(const std::vector<AStarNode*>& v) const;
};

class AStarGrid {
public:
    AStarGrid(int w, int h);
    ~AStarGrid();
    int width() const { return m_w; }
    int height() const { return m_h; }
    AStarNode* nodeAt(int x, int y) const;
    // AStarGrid::findPath: shortest route over empty nodes (A* with costToNode as heuristic); empty when unreachable
    bool findPath(AStarNode* from, AStarNode* to, std::vector<AStarNode*>& path) const;
    std::vector<AStarNode*> sameTypeConnected(AStarNode* n) const;
    // AStarGrid::neighborsToNode(node, radius)
    std::vector<AStarNode*> neighborsToNode(AStarNode* n, int radius) const;
    // AStarGrid::validMoveExists: some movable piece has an empty neighbour
    bool validMoveExists() const;
    AStarNode* nodeAtPoint(float x, float y, float cell, float ox, float oy) const;
private:
    int m_w, m_h;
    std::vector<AStarNode*> m_nodes;
};
