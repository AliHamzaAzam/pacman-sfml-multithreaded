#include "Constants.h"
#include "Maze.h"
#include "Pacman.h"
#include "Ghost.h"
#include "ThreadManager.h"
#include "Menu.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <unistd.h>
#include <atomic>

// Global flags for thread coordination
std::atomic<bool> gameStarted(false);
std::atomic<bool> gamePaused(false);

// Collision detection between Pac-Man and ghosts
// Returns: 0 = no collision, 1 = Pac-Man eats ghost, -1 = ghost catches Pac-Man
int checkCollision(Pacman& pacman, Ghost& ghost) {
    float dx = pacman.x - ghost.x;
    float dy = pacman.y - ghost.y;
    float distSq = dx*dx + dy*dy;
    float collisionDist = 20.0f; // Collision radius
    
    if (distSq < collisionDist * collisionDist) {
        if (ghost.state == GhostState::IN_HOUSE) {
            return 0; // No collision with ghosts in house
        }
        if (ghost.state == GhostState::FRIGHTENED) {
            return 1; // Pac-Man eats ghost
        }
        if (ghost.state == GhostState::EATEN) {
            return 0; // Already eaten, no collision
        }
        return -1; // Ghost catches Pac-Man
    }
    return 0;
}
// Game engine thread function - handles Pac-Man movement, collision, etc.
void* gameEngineThread(void* arg) {
    GameThreadData* data = static_cast<GameThreadData*>(arg);
    
    while (data->running) {
        // Lock mutex before modifying game state
        pthread_mutex_lock(data->gameMutex);
        
        // Note: actual update happens in main loop for now
        // This thread will handle collision detection
        
        pthread_mutex_unlock(data->gameMutex);
        
        usleep(16000); // ~60 FPS (16ms)
    }
    
    std::cout << "Game engine thread exiting" << std::endl;
    return nullptr;
}

// Ghost controller thread function
void* ghostControllerThread(void* arg) {
    GhostThreadData* data = static_cast<GhostThreadData*>(arg);
    
    // Wait for game to start (menu to transition to PLAYING)
    while (!gameStarted.load() && data->running) {
        usleep(100000);  // Check every 100ms
    }
    if (!data->running) return nullptr;
    
    // Initial spawn: wait for staggered exit timing
    int waitMs = (data->ghostIndex + 1) * 3000;  // 3s, 6s, 9s, 12s in 100ms chunks
    int waited = 0;
    while (waited < waitMs && data->running) {
        // Only count time when game is actively playing (not paused)
        if (!gamePaused.load()) {
            waited += 100;
        }
        usleep(100000);  // 100ms
    }
    if (!data->running) return nullptr;
    
    // Wait until game is unpaused to actually leave house
    while (gamePaused.load() && data->running) {
        usleep(100000);
    }
    if (!data->running) return nullptr;
    
    sem_wait(data->spawnSemaphore);
    if (!gamePaused.load()) {  // Double-check not paused
        std::cout << "Ghost " << data->ghostIndex << " leaving house" << std::endl;
        data->ghost->leaveHouse();
    }
    sem_post(data->spawnSemaphore);
    
    while (data->running) {
        // Skip all processing while game is paused
        if (gamePaused.load()) {
            usleep(100000);  // Check every 100ms
            continue;
        }
        
        pthread_mutex_lock(data->gameMutex);
        
        // Check if ghost returned to house (after being eaten)
        if (data->ghost->state == GhostState::IN_HOUSE && data->ghost->inHouse) {
            // Wait until Pac-Man is not powered up
            bool pacmanPowered = data->pacman->powered;
            pthread_mutex_unlock(data->gameMutex);
            
            // Don't exit while Pac-Man is powered
            if (pacmanPowered) {
                usleep(100000); // Check again in 100ms
                continue;
            }
            
            // Staggered exit timing (same as game start)
            usleep((data->ghostIndex + 1) * 3000000); // 3s, 6s, 9s, 12s
            
            sem_wait(data->spawnSemaphore);
            pthread_mutex_lock(data->gameMutex);
            // Double-check not powered and still in house
            if (data->ghost->state == GhostState::IN_HOUSE && !data->pacman->powered) {
                std::cout << "Ghost " << data->ghostIndex << " reviving" << std::endl;
                data->ghost->leaveHouse();
            }
            pthread_mutex_unlock(data->gameMutex);
            sem_post(data->spawnSemaphore);
        } else {
            pthread_mutex_unlock(data->gameMutex);
        }
        
        usleep(20000); // ~50 FPS
    }
    
    std::cout << "Ghost " << data->ghostIndex << " thread exiting" << std::endl;
    return nullptr;
}

void drawMaze(sf::RenderWindow& window, const Maze& maze, sf::Sprite& mazeSprite) {
    window.draw(mazeSprite);
    
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

void drawNodeGraph(sf::RenderWindow& window, const Maze& maze) {
    sf::CircleShape nodeMarker(4.0f);
    nodeMarker.setFillColor(sf::Color::Green);
    nodeMarker.setOrigin(sf::Vector2f(4.0f, 4.0f));
    
    for (size_t i = 0; i < maze.nodeCount(); i++) {
        const Node& node = maze.getNode(i);
        float x1 = maze.nodeX(i);
        float y1 = maze.nodeY(i);
        
        for (int d = 0; d < 2; d++) {
            if (node.neighbors[d] >= 0) {
                int n2 = node.neighbors[d];
                // Skip tunnel connections in visualization
                float dx = std::abs(maze.nodeX(n2) - x1);
                if (dx > 400.0f) continue;
                
                sf::Vertex line[] = {
                    sf::Vertex{sf::Vector2f(x1, y1), sf::Color(50, 200, 50, 180)},
                    sf::Vertex{sf::Vector2f(maze.nodeX(n2), maze.nodeY(n2)), sf::Color(50, 200, 50, 180)}
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }
    }
    
    for (size_t i = 0; i < maze.nodeCount(); i++) {
        nodeMarker.setPosition(sf::Vector2f(maze.nodeX(i), maze.nodeY(i)));
        window.draw(nodeMarker);
    }
}

void drawScore(sf::RenderWindow& window, const Pacman& pacman, sf::Font& font) {
    sf::Text scoreText(font);
    scoreText.setString("Score: " + std::to_string(pacman.score));
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(sf::Vector2f(10.f, 10.f));
    window.draw(scoreText);
    
    sf::Text livesText(font);
    livesText.setString("Lives: " + std::to_string(pacman.lives));
    livesText.setCharacterSize(24);
    livesText.setFillColor(sf::Color::White);
    livesText.setPosition(sf::Vector2f(Config::WINDOW_WIDTH - 120.f, 10.f));
    window.draw(livesText);
}

void resetGame(Maze& maze, Pacman& pacman, Ghost* ghosts[4]) {
    // Reset maze (restore coins)
    maze.initialize();
    
    // Reset Pac-Man
    pacman.spawn(maze);
    pacman.score = 0;
    pacman.lives = 3;
    pacman.powered = false;
    pacman.powerTimer = 0;
    
    // Reset ghosts
    for (int i = 0; i < 4; i++) {
        ghosts[i]->spawn(maze, maze.getGhostSpawnNode(i));
    }
}

int main() {
#ifdef __APPLE__
    setenv("SFML_SILENCE_MACOS_KEYBOARD_WARNING", "1", 1);
#endif

    // Initialize thread manager
    ThreadManager threadManager;
    if (!threadManager.initialize()) {
        std::cerr << "Failed to initialize thread manager" << std::endl;
        return 1;
    }

    // Load resources
    sf::Texture mazeTexture;
    if (!mazeTexture.loadFromFile("resources/maze.png")) {
        std::cerr << "Failed to load maze.png" << std::endl;
        return 1;
    }
    sf::Sprite mazeSprite(mazeTexture);
    mazeSprite.setScale(sf::Vector2f(Maze::SCALE, Maze::SCALE));
    mazeSprite.setPosition(sf::Vector2f(Maze::OFFSET_X, Maze::OFFSET_Y));
    
    sf::Font font;
    if (!font.openFromFile("resources/PAC-FONT.TTF")) {
        std::cerr << "Warning: Could not load font, score display disabled" << std::endl;
    }
    
    // Initialize game objects
    Maze maze;
    maze.initialize();
    
    Pacman pacman;
    pacman.spawn(maze);
    
    Ghost blinky(GhostType::BLINKY);
    Ghost pinky(GhostType::PINKY);
    Ghost inky(GhostType::INKY);
    Ghost clyde(GhostType::CLYDE);
    
    blinky.spawn(maze, maze.getGhostSpawnNode(0));
    pinky.spawn(maze, maze.getGhostSpawnNode(1));
    inky.spawn(maze, maze.getGhostSpawnNode(2));
    clyde.spawn(maze, maze.getGhostSpawnNode(3));
    
    Ghost* ghosts[4] = {&blinky, &pinky, &inky, &clyde};
    
    std::cout << "Maze: " << maze.nodeCount() << " nodes, " << maze.getTotalCoins() << " coins" << std::endl;
    std::cout << "Pacman at node " << pacman.currentNode << std::endl;
    
    // Setup thread data
    threadManager.setupGameThread(&maze, &pacman, ghosts, 4);
    for (int i = 0; i < 4; i++) {
        threadManager.setupGhostThread(i, &maze, &pacman, ghosts[i]);
    }
    
    // Create threads
    pthread_create(&threadManager.gameThread, nullptr, gameEngineThread, &threadManager.gameData);
    for (int i = 0; i < 4; i++) {
        pthread_create(&threadManager.ghostThreads[i], nullptr, ghostControllerThread, &threadManager.ghostData[i]);
    }
    
    std::cout << "Started 5 threads (1 engine + 4 ghosts)" << std::endl;
    
    // Create window
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT)),
        "Pac-Man (Multithreaded)"
    );
    window.setFramerateLimit(Config::FPS);
    
    bool showDebug = false;
    Menu menu;
    sf::Clock clock;
    
    std::cout << "Controls: Arrows=move, P=pause, D=debug" << std::endl;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        // Event handling (main thread only)
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (event->is<sf::Event::KeyPressed>()) {
                auto keyEvent = event->getIf<sf::Event::KeyPressed>();
                
                // Handle menu input
                menu.handleInput(keyEvent->code);
                
                // Check for exit 
                if (keyEvent->code == sf::Keyboard::Key::Enter && menu.shouldExit()) {
                    window.close();
                }
                
                // Game controls only when playing
                if (menu.state == MenuState::PLAYING) {
                    switch (keyEvent->code) {
                        case sf::Keyboard::Key::Right:
                            pthread_mutex_lock(&threadManager.gameMutex);
                            pacman.setDirection(DIR_RIGHT);
                            pthread_mutex_unlock(&threadManager.gameMutex);
                            break;
                        case sf::Keyboard::Key::Down:
                            pthread_mutex_lock(&threadManager.gameMutex);
                            pacman.setDirection(DIR_DOWN);
                            pthread_mutex_unlock(&threadManager.gameMutex);
                            break;
                        case sf::Keyboard::Key::Left:
                            pthread_mutex_lock(&threadManager.gameMutex);
                            pacman.setDirection(DIR_LEFT);
                            pthread_mutex_unlock(&threadManager.gameMutex);
                            break;
                        case sf::Keyboard::Key::Up:
                            pthread_mutex_lock(&threadManager.gameMutex);
                            pacman.setDirection(DIR_UP);
                            pthread_mutex_unlock(&threadManager.gameMutex);
                            break;
                        case sf::Keyboard::Key::D:
                            showDebug = !showDebug;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        
        // Update game state (protected by mutex)
        pthread_mutex_lock(&threadManager.gameMutex);
        
        // Handle menu requests
        if (menu.requestNewGame) {
            menu.requestNewGame = false;
            resetGame(maze, pacman, ghosts);
            gameStarted.store(false);  // Reset for ghost threads
        }
        if (menu.requestExitToMenu) {
            menu.requestExitToMenu = false;
            // Keep game state for Continue option
        }
        
        if (menu.state == MenuState::PLAYING) {
            gamePaused.store(false);  // Unpauses ghost threads
            menu.hasActiveGame = true;  // Mark that game is in progress
            
            // Signal threads that game has started
            if (!gameStarted.load()) {
                gameStarted.store(true);
            }
            pacman.update(maze, dt);
            for (Ghost* ghost : ghosts) {
                ghost->update(maze, pacman, dt);
            }
            
            // Check collisions
            for (Ghost* ghost : ghosts) {
                int collision = checkCollision(pacman, *ghost);
                if (collision == 1) {
                    // Pac-Man eats ghost
                    ghost->state = GhostState::EATEN;
                    pacman.score += 200;
                    std::cout << "Ghost eaten! Score: " << pacman.score << std::endl;
                }
                else if (collision == -1) {
                    // Ghost catches Pac-Man
                    pacman.lives--;
                    std::cout << "Pac-Man caught! Lives: " << pacman.lives << std::endl;
                    
                    if (pacman.lives <= 0) {
                        menu.state = MenuState::GAME_OVER;
                        std::cout << "GAME OVER!" << std::endl;
                    } else {
                        // Reset positions
                        pacman.spawn(maze);
                        for (int i = 0; i < 4; i++) {
                            ghosts[i]->spawn(maze, maze.getGhostSpawnNode(i));
                            ghosts[i]->state = GhostState::CHASE; // Don't go back to house
                        }
                    }
                }
            }
            
            // Check win condition
            if (maze.allCoinsCollected()) {
                menu.state = MenuState::WIN;
                std::cout << "YOU WIN! Score: " << pacman.score << std::endl;
            }
        } else {
            // Game not playing - pause ghost threads
            gamePaused.store(true);
        }
        
        pthread_mutex_unlock(&threadManager.gameMutex);
        
        // Render (main thread only - SFML requirement)
        window.clear(sf::Color::Black);
        
        pthread_mutex_lock(&threadManager.gameMutex);
        
        // Draw maze background
        window.draw(mazeSprite);
        
        // Only draw game entities when not in main menu
        if (menu.state != MenuState::MAIN_MENU) {
            drawMaze(window, maze, mazeSprite);
            if (showDebug) {
                drawNodeGraph(window, maze);
            }
            window.draw(pacman.sprite);
            for (Ghost* ghost : ghosts) {
                window.draw(ghost->sprite);
            }
        }
        
        menu.draw(window, pacman.score, pacman.lives);
        pthread_mutex_unlock(&threadManager.gameMutex);
        
        window.display();
    }

    // Stop all threads
    threadManager.stopAll();
    
    // Wait for threads to finish
    pthread_join(threadManager.gameThread, nullptr);
    for (int i = 0; i < 4; i++) {
        pthread_join(threadManager.ghostThreads[i], nullptr);
    }
    
    std::cout << "All threads joined" << std::endl;
    std::cout << "Final score: " << pacman.score << std::endl;
    
    return 0;
}
