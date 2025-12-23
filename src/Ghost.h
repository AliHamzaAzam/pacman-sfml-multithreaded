#ifndef GHOST_H
#define GHOST_H

#include "Maze.h"
#include "Pacman.h"
#include <SFML/Graphics.hpp>
#include <random>

enum class GhostState {
    IN_HOUSE,
    LEAVING_HOUSE,
    CHASE,
    SCATTER,
    FRIGHTENED,
    EATEN
};

enum class GhostType {
    BLINKY,  // Red - chases directly
    PINKY,   // Pink - ambushes ahead
    INKY,    // Cyan - unpredictable
    CLYDE    // Orange - shy, random
};

class Ghost {
public:
    // Node-based position
    int currentNode;
    int targetNode;
    Direction direction;
    
    // Pixel position
    float x, y;
    float baseSpeed;
    float speed;
    
    // State
    GhostType type;
    GhostState state;
    bool inHouse;
    
    // AI mode switching (chase <-> scatter)
    float modeTimer;
    float scatterTargetX, scatterTargetY;  // Corner target for scatter mode
    float blinkTimer;  // For frightened blinking animation
    
    // SFML
    sf::Texture texture;
    sf::Sprite sprite;
    int textureOffsetX;
    
    Ghost(GhostType ghostType) : sprite(texture), type(ghostType),
                                  currentNode(0), targetNode(-1),
                                  direction(DIR_NONE),
                                  x(0), y(0), speed(1.5f), baseSpeed(1.5f),
                                  state(GhostState::IN_HOUSE), inHouse(true),
                                  modeTimer(0), scatterTargetX(0), scatterTargetY(0),
                                  blinkTimer(0) {
        
        if (!texture.loadFromFile("resources/sprites.png")) {
            // Handle error
        }
        
        // Set speed and scatter corner based on type
        // Corners at ~(50,60), (600,60), (50,700), (600,700)
        switch (type) {
            case GhostType::BLINKY:
                textureOffsetX = 0;
                baseSpeed = 1.8f;
                scatterTargetX = 600.0f; scatterTargetY = 60.0f;  // Top-right
                break;
            case GhostType::PINKY:
                textureOffsetX = 50;
                baseSpeed = 1.6f;
                scatterTargetX = 50.0f; scatterTargetY = 60.0f;   // Top-left
                break;
            case GhostType::INKY:
                textureOffsetX = 100;
                baseSpeed = 1.5f;
                scatterTargetX = 600.0f; scatterTargetY = 700.0f; // Bottom-right
                break;
            case GhostType::CLYDE:
                textureOffsetX = 150;
                baseSpeed = 1.4f;
                scatterTargetX = 50.0f; scatterTargetY = 700.0f;  // Bottom-left
                break;
        }
        speed = baseSpeed;
        
        sprite.setTextureRect(sf::IntRect(sf::Vector2i(textureOffsetX, 0), sf::Vector2i(50, 50)));
        sprite.setOrigin(sf::Vector2f(25.0f, 25.0f));
    }
    
    void spawn(const Maze& maze, int nodeId) {
        currentNode = nodeId;
        targetNode = -1;
        x = maze.nodeX(currentNode);
        y = maze.nodeY(currentNode);
        sprite.setPosition(sf::Vector2f(x, y));
        state = GhostState::IN_HOUSE;
        inHouse = true;
    }
    
    void leaveHouse() {
        if (state == GhostState::IN_HOUSE) {
            state = GhostState::LEAVING_HOUSE;
            inHouse = false;
        }
    }
    
    void update(const Maze& maze, const Pacman& pacman, float dt) {
        // Bounce up/down while in ghost house
        if (state == GhostState::IN_HOUSE) {
            static float bounceTimer = 0;
            bounceTimer += dt * 2.0f;
            float baseY = maze.nodeY(currentNode);
            y = baseY + std::sin(bounceTimer + currentNode) * 10.0f;
            updateSprite(pacman.powered);
            return;
        }
        
        // State-based speed and timers
        switch (state) {
            case GhostState::FRIGHTENED:
                speed = baseSpeed * 0.5f;
                blinkTimer += dt;  // For blinking animation
                break;
            case GhostState::EATEN:
                speed = baseSpeed * 2.0f;
                blinkTimer = 0;
                break;
            default:
                speed = baseSpeed;
                blinkTimer = 0;
                break;
        }
        
        // Mode switching: alternate between chase and scatter
        // Chase for 20s, scatter for 7s (like classic Pac-Man)
        if (state == GhostState::CHASE || state == GhostState::SCATTER) {
            modeTimer += dt;
            float switchTime = (state == GhostState::CHASE) ? 20.0f : 7.0f;
            if (modeTimer >= switchTime) {
                modeTimer = 0;
                state = (state == GhostState::CHASE) ? GhostState::SCATTER : GhostState::CHASE;
                // Reverse direction on mode switch
                direction = oppositeDir(direction);
            }
        }
        
        // Handle frightened state from Pacman power-up
        if (pacman.powered && state != GhostState::EATEN && state != GhostState::IN_HOUSE) {
            if (state != GhostState::FRIGHTENED) {
                state = GhostState::FRIGHTENED;
                modeTimer = 0;  // Reset timer
                // Reverse direction
                direction = oppositeDir(direction);
                if (targetNode >= 0) {
                    int temp = currentNode;
                    currentNode = targetNode;
                    targetNode = temp;
                }
            }
        } else if (!pacman.powered && state == GhostState::FRIGHTENED) {
            state = GhostState::CHASE;
        }
        
        // At a node - choose next direction
        if (targetNode < 0) {
            const Node& node = maze.getNode(currentNode);
            Direction newDir = chooseDirection(maze, pacman, node);
            
            if (newDir != DIR_NONE && node.hasNeighbor(newDir)) {
                int nextNode = node.neighbors[newDir];
                // Only EATEN ghosts can enter ghost house (25 → 32)
                if (currentNode == 25 && nextNode == 32 && state != GhostState::EATEN) {
                    // Block entry, try another direction
                } else {
                    direction = newDir;
                    targetNode = nextNode;
                }
            }
        }
        
        // Move towards target
        if (targetNode >= 0) {
            float targetX = maze.nodeX(targetNode);
            float targetY = maze.nodeY(targetNode);
            float dx = targetX - x;
            float dy = targetY - y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            // Tunnel teleportation (distance > 400 = teleport)
            if (dist > 400.0f) {
                x = targetX;
                y = targetY;
                currentNode = targetNode;
                targetNode = -1;
            }
            else if (dist < speed) {
                x = targetX;
                y = targetY;
                currentNode = targetNode;
                targetNode = -1;
            } else {
                x += (dx / dist) * speed;
                y += (dy / dist) * speed;
            }
        }
        
        // Eaten ghost reached house? Revive with delay
        if (state == GhostState::EATEN) {
            // Only revive when reaching specific house nodes: 75, 32, or 34
            if (currentNode == 75 || currentNode == 32 || currentNode == 34) {
                // Back in house - wait for semaphore exit (via thread)
                state = GhostState::IN_HOUSE;
                inHouse = true;
            }
        }
        
        // Ghost leaving house reached exit? Start chasing
        if (state == GhostState::LEAVING_HOUSE && currentNode == 25) {
            state = GhostState::CHASE;
            modeTimer = 0;
        }
        
        updateSprite(pacman.powered);
    }
    
    void setFrightened() {
        if (state != GhostState::IN_HOUSE && state != GhostState::EATEN) {
            state = GhostState::FRIGHTENED;
        }
    }
    
    void reset(const Maze& maze, int nodeId) {
        spawn(maze, nodeId);
        state = GhostState::IN_HOUSE;
        inHouse = true;
    }
    
private:
    Direction chooseDirection(const Maze& maze, const Pacman& pacman, const Node& node) {
        Direction opposite = oppositeDir(direction);
        
        // Collect available directions (excluding reverse, unless only option)
        std::vector<Direction> available;
        for (int d = 0; d < 4; d++) {
            Direction dir = static_cast<Direction>(d);
            if (node.hasNeighbor(dir) && dir != opposite) {
                // Block ghost house entry (node 25 -> 32) for non-EATEN ghosts
                if (currentNode == 25 && node.neighbors[dir] == 32 && state != GhostState::EATEN) {
                    continue;  // Skip this direction
                }
                available.push_back(dir);
            }
        }
        
        if (available.empty()) {
            // Dead end, must reverse
            if (node.hasNeighbor(opposite)) {
                return opposite;
            }
            return DIR_NONE;
        }
        
        if (available.size() == 1) {
            return available[0];
        }
        
        // Choose based on state and ghost type
        switch (state) {
            case GhostState::FRIGHTENED:
                return fleeDirection(maze, pacman, node, available);
                
            case GhostState::CHASE:
                return chaseDirection(maze, pacman, node, available);
                
            case GhostState::SCATTER:
                return scatterDirection(maze, node, available);
            
            case GhostState::EATEN:
                return eatenDirection(maze, node, available);
            
            case GhostState::LEAVING_HOUSE:
                return leavingHouseDirection(maze, node, available);
                
            default:
                return chaseDirection(maze, pacman, node, available);
        }
    }
    
    Direction eatenDirection(const Maze& maze, const Node& node, 
                             const std::vector<Direction>& available) {
        // Target ghost house nodes: 75, 32, or 34
        // Find which is closest and navigate there
        static const int houseNodes[] = {75, 32, 34};
        float bestHouseDist = 999999.0f;
        int targetHouseNode = 32;
        
        for (int hn : houseNodes) {
            float dx = maze.nodeX(hn) - x;
            float dy = maze.nodeY(hn) - y;
            float dist = dx*dx + dy*dy;
            if (dist < bestHouseDist) {
                bestHouseDist = dist;
                targetHouseNode = hn;
            }
        }
        
        float houseX = maze.nodeX(targetHouseNode);
        float houseY = maze.nodeY(targetHouseNode);
        
        Direction best = available[0];
        float bestDist = 999999.0f;
        
        for (Direction dir : available) {
            int neighborId = node.neighbors[dir];
            float dx = maze.nodeX(neighborId) - houseX;
            float dy = maze.nodeY(neighborId) - houseY;
            float dist = dx*dx + dy*dy;
            
            if (dist < bestDist) {
                bestDist = dist;
                best = dir;
            }
        }
        return best;
    }
    
    Direction fleeDirection(const Maze& maze, const Pacman& pacman, const Node& node,
                           const std::vector<Direction>& available) {
        // Move AWAY from Pac-Man (maximize distance)
        float pacX = pacman.x;
        float pacY = pacman.y;
        
        Direction best = available[0];
        float bestDist = -1.0f;  // We want maximum distance
        
        for (Direction dir : available) {
            int neighborId = node.neighbors[dir];
            float dx = maze.nodeX(neighborId) - pacX;
            float dy = maze.nodeY(neighborId) - pacY;
            float dist = dx*dx + dy*dy;  // Distance squared
            
            if (dist > bestDist) {  // Find MAXIMUM distance
                bestDist = dist;
                best = dir;
            }
        }
        return best;
    }
    
    Direction leavingHouseDirection(const Maze& maze, const Node& node, 
                                     const std::vector<Direction>& available) {
        // Target node 25 (ghost house exit)
        float exitX = maze.nodeX(25);
        float exitY = maze.nodeY(25);
        
        Direction best = available[0];
        float bestDist = 999999.0f;
        
        for (Direction dir : available) {
            int neighborId = node.neighbors[dir];
            float dx = maze.nodeX(neighborId) - exitX;
            float dy = maze.nodeY(neighborId) - exitY;
            float dist = dx*dx + dy*dy;
            
            if (dist < bestDist) {
                bestDist = dist;
                best = dir;
            }
        }
        return best;
    }
    
    Direction scatterDirection(const Maze& maze, const Node& node, 
                               const std::vector<Direction>& available) {
        // Move towards assigned corner
        Direction best = available[0];
        float bestDist = 999999.0f;
        
        for (Direction dir : available) {
            int neighborId = node.neighbors[dir];
            float dx = maze.nodeX(neighborId) - scatterTargetX;
            float dy = maze.nodeY(neighborId) - scatterTargetY;
            float dist = dx*dx + dy*dy;
            
            if (dist < bestDist) {
                bestDist = dist;
                best = dir;
            }
        }
        return best;
    }
    
    Direction chaseDirection(const Maze& maze, const Pacman& pacman, 
                             const Node& node, const std::vector<Direction>& available) {
        float targetX = pacman.x;
        float targetY = pacman.y;
        
        // Different targeting for each ghost type
        switch (type) {
            case GhostType::BLINKY:
                // Direct chase
                break;
                
            case GhostType::PINKY:
                // Target 4 tiles ahead of Pacman
                switch (pacman.direction) {
                    case DIR_UP:    targetY -= 80.0f; break;
                    case DIR_DOWN:  targetY += 80.0f; break;
                    case DIR_LEFT:  targetX -= 80.0f; break;
                    case DIR_RIGHT: targetX += 80.0f; break;
                    default: break;
                }
                break;
                
            case GhostType::INKY:
            case GhostType::CLYDE:
            default:
                // Random factor for unpredictability
                if (rand() % 4 == 0) {
                    return randomDirection(available);
                }
                break;
        }
        
        // Find direction that gets closest to target
        Direction best = available[0];
        float bestDist = 999999.0f;
        
        for (Direction dir : available) {
            int neighborId = node.neighbors[dir];
            float dx = maze.nodeX(neighborId) - targetX;
            float dy = maze.nodeY(neighborId) - targetY;
            float dist = dx*dx + dy*dy;
            
            if (dist < bestDist) {
                bestDist = dist;
                best = dir;
            }
        }
        
        return best;
    }
    
    Direction randomDirection(const std::vector<Direction>& available) {
        if (available.empty()) return DIR_NONE;
        return available[rand() % available.size()];
    }
    
    void updateSprite(bool pacmanPowered) {
        int scared = (state == GhostState::FRIGHTENED) ? 550 : 0;
        int texX = (state == GhostState::FRIGHTENED) ? 0 : textureOffsetX;
        
        // Blinking when frightened (alternates every 0.25s)
        if (state == GhostState::FRIGHTENED) {
            bool blink = (static_cast<int>(blinkTimer * 4) % 2) == 0;
            if (blink) {
                texX = 50;  // White/flashing texture
            }
        }
        
        int dirOffset = 0;
        switch (direction) {
            case DIR_RIGHT: dirOffset = 50; break;
            case DIR_DOWN:  dirOffset = 100; break;
            case DIR_LEFT:  dirOffset = 200; break;
            case DIR_UP:    dirOffset = 300; break;
            default:        dirOffset = 0; break;
        }
        
        sprite.setTextureRect(sf::IntRect(
            sf::Vector2i(texX, dirOffset + scared),
            sf::Vector2i(50, 50)
        ));
        sprite.setPosition(sf::Vector2f(x, y));
    }
};

#endif // GHOST_H
