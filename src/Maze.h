#ifndef MAZE_H
#define MAZE_H

#include "Constants.h"
#include "maze_data.h"
#include <vector>
#include <array>
#include <cmath>
#include <iostream>

// Direction indices for neighbor array
enum Direction { 
    DIR_RIGHT = 0, 
    DIR_DOWN = 1, 
    DIR_LEFT = 2, 
    DIR_UP = 3,
    DIR_NONE = -1
};

// Get opposite direction
inline Direction oppositeDir(Direction d) {
    switch(d) {
        case DIR_RIGHT: return DIR_LEFT;
        case DIR_LEFT: return DIR_RIGHT;
        case DIR_UP: return DIR_DOWN;
        case DIR_DOWN: return DIR_UP;
        default: return DIR_NONE;
    }
}

// Content type for nodes
enum class CellContent {
    EMPTY,
    COIN,
    POWER_PELLET
};

struct Node {
    int id = -1;
    int gridX = 0, gridY = 0;
    int pixelX = 0, pixelY = 0;          // Pre-computed pixel coordinates
    std::array<int, 4> neighbors;         // [RIGHT, DOWN, LEFT, UP], -1 if none
    CellContent content = CellContent::COIN;
    bool isGhostHouse = false;
    bool isTunnel = false;
    
    Node() { neighbors.fill(-1); }
    
    bool hasNeighbor(Direction dir) const {
        int d = static_cast<int>(dir);
        return d >= 0 && d < 4 && neighbors[d] != -1;
    }
    
    int countNeighbors() const {
        int count = 0;
        for (int n : neighbors) if (n != -1) count++;
        return count;
    }
};

class Maze {
public:
    // Scale factor: maze_data.h pixels to screen pixels
    static constexpr float SCALE = 0.68f;  // Adjust to fit window
    static constexpr float OFFSET_X = 10.0f;
    static constexpr float OFFSET_Y = 10.0f;
    
private:
    std::vector<Node> nodes;
    int totalCoins = 0;
    int coinsCollected = 0;
    
public:
    void initialize() {
        loadFromMazeData();
        markSpecialNodes();
        countCoins();
        std::cout << "Maze: " << nodes.size() << " nodes, " << totalCoins << " coins" << std::endl;
    }
    
    // Get pixel position for a node (scaled and offset)
    float nodeX(int nodeId) const { 
        return nodes[nodeId].pixelX * SCALE + OFFSET_X; 
    }
    float nodeY(int nodeId) const { 
        return nodes[nodeId].pixelY * SCALE + OFFSET_Y; 
    }
    
    // Get node by ID
    const Node& getNode(int id) const { return nodes[id]; }
    Node& getNode(int id) { return nodes[id]; }
    
    // Get all nodes
    const std::vector<Node>& getNodes() const { return nodes; }
    size_t nodeCount() const { return nodes.size(); }
    
    // Collect coin at node
    bool collectCoinAtNode(int nodeId) {
        if (nodeId < 0 || nodeId >= (int)nodes.size()) return false;
        if (nodes[nodeId].content == CellContent::COIN) {
            nodes[nodeId].content = CellContent::EMPTY;
            coinsCollected++;
            return true;
        }
        return false;
    }
    
    // Collect power pellet at node  
    bool collectPowerPellet(int nodeId) {
        if (nodeId < 0 || nodeId >= (int)nodes.size()) return false;
        if (nodes[nodeId].content == CellContent::POWER_PELLET) {
            nodes[nodeId].content = CellContent::EMPTY;
            coinsCollected++;  // Count towards win condition
            return true;
        }
        return false;
    }
    
    // Check win condition
    bool allCoinsCollected() const { return coinsCollected >= totalCoins; }
    int getTotalCoins() const { return totalCoins; }
    int getCoinsCollected() const { return coinsCollected; }
    
    // Find nearest node to a pixel position
    int findNearestNode(float px, float py) const {
        int nearest = -1;
        float minDist = 999999.0f;
        for (size_t i = 0; i < nodes.size(); i++) {
            float dx = nodeX(i) - px;
            float dy = nodeY(i) - py;
            float dist = dx*dx + dy*dy;
            if (dist < minDist) {
                minDist = dist;
                nearest = i;
            }
        }
        return nearest;
    }
    
    // Get Pac-Man spawn node (bottom center, around row 23)
    int getPacmanSpawnNode() const {
        // Look for node around grid position (13-14, 23)
        for (const auto& node : nodes) {
            if (node.gridY == 23 && node.gridX >= 12 && node.gridX <= 15) {
                return node.id;
            }
        }
        return 53;  // Fallback: node 53 is at grid (12, 23)
    }
    
    // Get ghost house exit node (above ghost box, row 11)
    int getGhostHouseExitNode() const {
        for (const auto& node : nodes) {
            if (node.gridY == 11 && node.gridX >= 14 && node.gridX <= 15) {
                return node.id;
            }
        }
        return 25;  // Fallback: node 25 at (14, 11)
    }
    
    // Get ghost spawn nodes inside ghost house
    int getGhostSpawnNode(int ghostIndex) const {
        // Nodes inside or near ghost house (around row 14)
        static const int ghostNodes[] = {32, 34, 75, 32};  // Blinky, Pinky, Inky, Clyde
        return ghostNodes[ghostIndex % 4];
    }
    
private:
    void loadFromMazeData() {
        auto mazeNodes = getMazeNodes();
        nodes.resize(mazeNodes.size());
        
        for (size_t i = 0; i < mazeNodes.size(); i++) {
            const auto& md = mazeNodes[i];
            nodes[i].id = i;
            nodes[i].gridX = md.gridX;
            nodes[i].gridY = md.gridY;
            nodes[i].pixelX = md.pixelX;
            nodes[i].pixelY = md.pixelY;
            nodes[i].neighbors = md.neighbors;
            nodes[i].content = CellContent::COIN;  // Default: all nodes have coins
        }
    }
    
    void markSpecialNodes() {
        for (auto& node : nodes) {
            // Power pellets at corners (nodes at grid positions near corners)
            bool isCorner = (node.gridX <= 4 || node.gridX >= 24) && 
                           (node.gridY <= 5 || node.gridY >= 26);
            if (isCorner && node.countNeighbors() == 2) {
                node.content = CellContent::POWER_PELLET;
            }
            
            // Ghost house interior only (grid rows 12-16, cols 11-17) - no coins
            // This is more restrictive to keep the entrance (node 25) and surroundings having pellets
            if (node.gridY >= 12 && node.gridY <= 16 && 
                node.gridX >= 11 && node.gridX <= 17) {
                node.isGhostHouse = true;
                node.content = CellContent::EMPTY;  // No coins in ghost house interior
            }
            
            // Mark tunnel nodes but keep their coins
            if (node.gridX <= 1 || node.gridX >= 26) {
                node.isTunnel = true;
            }
        }
    }
    
    void countCoins() {
        totalCoins = 0;
        for (const auto& node : nodes) {
            if (node.content == CellContent::COIN || 
                node.content == CellContent::POWER_PELLET) {
                totalCoins++;
            }
        }
    }
};

#endif // MAZE_H
