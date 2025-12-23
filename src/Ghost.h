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
    
    // SFML
    sf::Texture texture;
    sf::Sprite sprite;
    int textureOffsetX;
    
    Ghost(GhostType ghostType) : sprite(texture), type(ghostType),
                                  currentNode(0), targetNode(-1),
                                  direction(DIR_NONE),
                                  x(0), y(0), speed(1.5f), baseSpeed(1.5f),
                                  state(GhostState::IN_HOUSE), inHouse(true) {
        
        if (!texture.loadFromFile("resources/sprites.png")) {
            // Handle error
        }
        
        // Set color based on type
        switch (type) {
            case GhostType::BLINKY:
                textureOffsetX = 0;
                baseSpeed = 1.8f;
                break;
            case GhostType::PINKY:
                textureOffsetX = 50;
                baseSpeed = 1.6f;
                break;
            case GhostType::INKY:
                textureOffsetX = 100;
                baseSpeed = 1.5f;
                break;
            case GhostType::CLYDE:
                textureOffsetX = 150;
                baseSpeed = 1.4f;
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
        // State-based speed
        switch (state) {
            case GhostState::FRIGHTENED:
                speed = baseSpeed * 0.5f;
                break;
            case GhostState::EATEN:
                speed = baseSpeed * 2.0f;
                break;
            default:
                speed = baseSpeed;
                break;
        }
        
        // Handle frightened state from Pacman power-up
        if (pacman.powered && state != GhostState::EATEN && state != GhostState::IN_HOUSE) {
            if (state != GhostState::FRIGHTENED) {
                state = GhostState::FRIGHTENED;
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
                direction = newDir;
                targetNode = node.neighbors[direction];
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
                return randomDirection(available);
                
            case GhostState::CHASE:
                return chaseDirection(maze, pacman, node, available);
                
            case GhostState::SCATTER:
            default:
                return randomDirection(available);
        }
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
