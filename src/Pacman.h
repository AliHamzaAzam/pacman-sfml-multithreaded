#ifndef PACMAN_H
#define PACMAN_H

#include "Maze.h"
#include <SFML/Graphics.hpp>

class Pacman {
public:
    // Node-based position
    int currentNode;
    int targetNode;
    Direction direction;
    Direction queuedDirection;  // Buffered input for next node
    
    // Pixel position (for smooth movement between nodes)
    float x, y;
    float speed;
    
    // State
    int lives;
    int score;
    bool powered;
    float powerTimer;
    
    // Animation
    int animFrame;
    int animCounter;
    
    // SFML
    sf::Texture texture;
    sf::Sprite sprite;
    
    Pacman() : sprite(texture), currentNode(0), targetNode(-1),
               direction(DIR_NONE), queuedDirection(DIR_NONE),
               x(0), y(0), speed(2.0f),
               lives(3), score(0), powered(false), powerTimer(0),
               animFrame(0), animCounter(0) {
        
        if (!texture.loadFromFile("resources/sprites.png")) {
            // Handle error
        }
        sprite.setTextureRect(sf::IntRect(sf::Vector2i(850, 0), sf::Vector2i(50, 50)));
        sprite.setOrigin(sf::Vector2f(25.0f, 25.0f));  // Center origin
    }
    
    void spawn(const Maze& maze) {
        currentNode = maze.getPacmanSpawnNode();
        targetNode = -1;
        direction = DIR_NONE;
        queuedDirection = DIR_NONE;
        
        const Node& node = maze.getNode(currentNode);
        x = maze.nodeX(currentNode);
        y = maze.nodeY(currentNode);
        sprite.setPosition(sf::Vector2f(x, y));
    }
    
    void setDirection(Direction dir) {
        queuedDirection = dir;
    }
    
    void update(Maze& maze, float dt) {
        // Update power-up timer
        if (powered) {
            powerTimer -= dt;
            if (powerTimer <= 0) {
                powered = false;
            }
        }
        
        // If at a node (not moving between nodes)
        if (targetNode < 0) {
            const Node& node = maze.getNode(currentNode);
            
            // Collect coin/pellet
            if (maze.collectCoinAtNode(currentNode)) {
                score += 10;
            }
            if (maze.collectPowerPellet(currentNode)) {
                score += 50;
                powered = true;
                powerTimer = 10.0f;  // 10 seconds
            }
            
            // Try queued direction first
            if (queuedDirection != DIR_NONE && node.hasNeighbor(queuedDirection)) {
                direction = queuedDirection;
                targetNode = node.neighbors[direction];
                queuedDirection = DIR_NONE;
            }
            // Otherwise continue in current direction if possible
            else if (direction != DIR_NONE && node.hasNeighbor(direction)) {
                targetNode = node.neighbors[direction];
            }
        }
        
        // Move towards target node
        if (targetNode >= 0) {
            float targetX = maze.nodeX(targetNode);
            float targetY = maze.nodeY(targetNode);
            float dx = targetX - x;
            float dy = targetY - y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            // Check for tunnel teleportation (target is far away horizontally)
            // If distance is > 400 pixels, it's a teleport
            if (dist > 400.0f) {
                // Instant teleport
                x = targetX;
                y = targetY;
                currentNode = targetNode;
                targetNode = -1;
            }
            else if (dist < speed) {
                // Arrived at target node
                x = targetX;
                y = targetY;
                currentNode = targetNode;
                targetNode = -1;
            } else {
                // Move towards target
                x += (dx / dist) * speed;
                y += (dy / dist) * speed;
            }
            
            // Animation
            animCounter++;
            if (animCounter > 5) {
                animFrame = (animFrame + 1) % 2;
                animCounter = 0;
            }
        }
        
        updateSprite();
    }
    
    void powerUp() {
        powered = true;
        powerTimer = 10.0f;
    }
    
    void die() {
        lives--;
    }
    
    void reset(const Maze& maze) {
        spawn(maze);
        direction = DIR_NONE;
        queuedDirection = DIR_NONE;
    }
    
private:
    void updateSprite() {
        // Texture offsets for each direction
        int frameOffset = animFrame * 50;
        int dirOffset = 0;
        
        switch (direction) {
            case DIR_RIGHT: dirOffset = 0; break;
            case DIR_DOWN:  dirOffset = 150; break;
            case DIR_LEFT:  dirOffset = 300; break;
            case DIR_UP:    dirOffset = 450; break;
            default:        dirOffset = 0; break;
        }
        
        sprite.setTextureRect(sf::IntRect(
            sf::Vector2i(850, dirOffset + frameOffset),
            sf::Vector2i(50, 50)
        ));
        sprite.setPosition(sf::Vector2f(x, y));
    }
};

#endif // PACMAN_H
