#include "Constants.h"
#include "Maze.h"
#include "Pacman.h"
#include "Ghost.h"

#include <SFML/Graphics.hpp>
#include <iostream>

void drawMaze(sf::RenderWindow& window, const Maze& maze, sf::Sprite& mazeSprite) {
    // Draw the maze background image
    window.draw(mazeSprite);
    
    // Draw coins at nodes
    sf::CircleShape coin(3.0f);
    coin.setFillColor(sf::Color::Yellow);
    coin.setOrigin(sf::Vector2f(3.0f, 3.0f));
    
    sf::CircleShape powerPellet(6.0f);
    powerPellet.setFillColor(sf::Color::Yellow);
    powerPellet.setOrigin(sf::Vector2f(6.0f, 6.0f));
    
    for (size_t i = 0; i < maze.nodeCount(); i++) {
        const Node& node = maze.getNode(i);
        if (node.content == CellContent::COIN) {
            coin.setPosition(sf::Vector2f(maze.nodeX(i), maze.nodeY(i)));
            window.draw(coin);
        } else if (node.content == CellContent::POWER_PELLET) {
            powerPellet.setPosition(sf::Vector2f(maze.nodeX(i), maze.nodeY(i)));
            window.draw(powerPellet);
        }
    }
}

// Debug visualization of nodes and connections
void drawNodeGraph(sf::RenderWindow& window, const Maze& maze) {
    sf::CircleShape nodeMarker(4.0f);
    nodeMarker.setFillColor(sf::Color::Green);
    nodeMarker.setOrigin(sf::Vector2f(4.0f, 4.0f));
    
    // Draw edges
    for (size_t i = 0; i < maze.nodeCount(); i++) {
        const Node& node = maze.getNode(i);
        float x1 = maze.nodeX(i);
        float y1 = maze.nodeY(i);
        
        // Only draw RIGHT and DOWN to avoid duplicates
        for (int d = 0; d < 2; d++) {
            if (node.neighbors[d] >= 0) {
                int n2 = node.neighbors[d];
                sf::Vertex line[] = {
                    sf::Vertex{sf::Vector2f(x1, y1), sf::Color(50, 200, 50, 180)},
                    sf::Vertex{sf::Vector2f(maze.nodeX(n2), maze.nodeY(n2)), sf::Color(50, 200, 50, 180)}
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }
    }
    
    // Draw nodes
    for (size_t i = 0; i < maze.nodeCount(); i++) {
        nodeMarker.setPosition(sf::Vector2f(maze.nodeX(i), maze.nodeY(i)));
        window.draw(nodeMarker);
    }
}

int main() {
#ifdef __APPLE__
    setenv("SFML_SILENCE_MACOS_KEYBOARD_WARNING", "1", 1);
#endif

    // Load maze texture
    sf::Texture mazeTexture;
    if (!mazeTexture.loadFromFile("resources/maze.png")) {
        std::cerr << "Failed to load maze.png" << std::endl;
        return 1;
    }
    sf::Sprite mazeSprite(mazeTexture);
    mazeSprite.setScale(sf::Vector2f(Maze::SCALE, Maze::SCALE));
    mazeSprite.setPosition(sf::Vector2f(Maze::OFFSET_X, Maze::OFFSET_Y));
    
    // Initialize maze
    Maze maze;
    maze.initialize();
    
    std::cout << "Maze initialized with " << maze.nodeCount() << " nodes" << std::endl;
    std::cout << "Total coins: " << maze.getTotalCoins() << std::endl;
    
    // Initialize Pacman
    Pacman pacman;
    pacman.spawn(maze);
    std::cout << "Pacman spawned at node " << pacman.currentNode << std::endl;
    
    // Initialize Ghosts
    Ghost blinky(GhostType::BLINKY);
    Ghost pinky(GhostType::PINKY);
    Ghost inky(GhostType::INKY);
    Ghost clyde(GhostType::CLYDE);
    
    blinky.spawn(maze, maze.getGhostSpawnNode(0));
    pinky.spawn(maze, maze.getGhostSpawnNode(1));
    inky.spawn(maze, maze.getGhostSpawnNode(2));
    clyde.spawn(maze, maze.getGhostSpawnNode(3));
    
    Ghost* ghosts[4] = {&blinky, &pinky, &inky, &clyde};
    
    // Create window
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT)),
        "Pac-Man"
    );
    window.setFramerateLimit(Config::FPS);
    
    bool showDebug = true;  // Toggle with D key
    sf::Clock clock;
    
    std::cout << "Game started. Controls: Arrow keys to move, D to toggle debug view" << std::endl;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        // Event handling
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (event->is<sf::Event::KeyPressed>()) {
                auto keyEvent = event->getIf<sf::Event::KeyPressed>();
                
                switch (keyEvent->code) {
                    case sf::Keyboard::Key::Right:
                        pacman.setDirection(DIR_RIGHT);
                        break;
                    case sf::Keyboard::Key::Down:
                        pacman.setDirection(DIR_DOWN);
                        break;
                    case sf::Keyboard::Key::Left:
                        pacman.setDirection(DIR_LEFT);
                        break;
                    case sf::Keyboard::Key::Up:
                        pacman.setDirection(DIR_UP);
                        break;
                    case sf::Keyboard::Key::D:
                        showDebug = !showDebug;
                        break;
                    case sf::Keyboard::Key::Escape:
                        window.close();
                        break;
                    default:
                        break;
                }
            }
        }
        
        // Update
        pacman.update(maze, dt);
        for (Ghost* ghost : ghosts) {
            ghost->update(maze, pacman, dt);
        }
        
        // Render
        window.clear(sf::Color::Black);
        
        drawMaze(window, maze, mazeSprite);
        
        if (showDebug) {
            drawNodeGraph(window, maze);
        }
        
        // Draw entities
        window.draw(pacman.sprite);
        for (Ghost* ghost : ghosts) {
            window.draw(ghost->sprite);
        }
        
        window.display();
    }

    std::cout << "Final score: " << pacman.score << std::endl;
    return 0;
}
