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
#include <climits>
#include <libgen.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#endif

// Make asset paths ("resources/...") resolve regardless of the launch CWD by
// switching to the directory that contains the executable. POSIX-only, which
// matches this project's threading model (macOS + Linux).
static void changeToExecutableDir() {
    char path[PATH_MAX];
#if defined(__APPLE__)
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return; // buffer too small
#else // Linux
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0) return;
    path[len] = '\0';
#endif
    char* dir = dirname(path); // may modify `path`; we don't use it afterwards
    if (dir) {
        if (chdir(dir) != 0) {
            std::cerr << "Warning: could not chdir to executable directory" << std::endl;
        }
    }
}

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
// Pac-Man controller thread function - handles Pac-Man movement
void* pacmanControllerThread(void* arg) {
    PacManThreadData* data = static_cast<PacManThreadData*>(arg);
    
    std::cout << "Pac-Man thread started" << std::endl;
    
    while (data->running) {
        // Wait for game to start and not be paused
        if (!gameStarted.load() || gamePaused.load()) {
            usleep(50000);  // 50ms
            continue;
        }
        
        pthread_mutex_lock(data->gameMutex);
        
        // Acquire WRITE lock before modifying board (eating pellets)
        pthread_rwlock_wrlock(data->boardLock);
        
        // Update Pac-Man position (includes eating pellets - write operation)
        float dt = 0.016f;  // ~60 FPS
        
        // Check if about to eat a power pellet 
        int currentNode = data->pacman->currentNode;
        bool hadPowerPellet = data->maze->hasPowerPellet(currentNode);
        
        data->pacman->update(*data->maze, dt);
        
        // If ate a power pellet, use semaphore
        if (hadPowerPellet && !data->maze->hasPowerPellet(currentNode)) {
            // Try to acquire power pellet semaphore (ensures controlled eating)
            if (sem_trywait(data->powerPelletSem) == 0) {
                std::cout << "Power pellet consumed (semaphore acquired)" << std::endl;
            }
        }
        
        pthread_rwlock_unlock(data->boardLock);
        pthread_mutex_unlock(data->gameMutex);
        
        usleep(16000);  // ~60 FPS
    }
    
    std::cout << "Pac-Man thread exiting" << std::endl;
    return nullptr;
}

// Game engine thread function - handles collisions, scoring, game state
void* gameEngineThread(void* arg) {
    GameThreadData* data = static_cast<GameThreadData*>(arg);
    
    std::cout << "Game engine thread started" << std::endl;
    
    while (data->running) {
        // Wait for game to start and not be paused
        if (!gameStarted.load() || gamePaused.load()) {
            usleep(50000);  // 50ms
            continue;
        }
        
        pthread_mutex_lock(data->gameMutex);
        
        // Acquire READ lock for ghost board reads
        pthread_rwlock_rdlock(data->boardLock);
        
        // Update all ghosts (read board state for pathfinding)
        float dt = 0.016f;
        for (int i = 0; i < 4; i++) {
            data->ghosts[i]->update(*data->maze, *data->pacman, dt);
        }
        
        pthread_rwlock_unlock(data->boardLock);
        
        // Check collisions between Pac-Man and all ghosts
        for (int i = 0; i < 4; i++) {
            int collision = checkCollision(*data->pacman, *data->ghosts[i]);
            if (collision == 1) {
                // Pac-Man eats ghost
                data->ghosts[i]->state = GhostState::EATEN;
                data->pacman->score += 200;
                std::cout << "Ghost " << i << " eaten! Score: " << data->pacman->score << std::endl;
            }
            else if (collision == -1) {
                // Ghost catches Pac-Man
                data->pacman->lives--;
                std::cout << "Pac-Man caught! Lives: " << data->pacman->lives << std::endl;
                
                if (data->pacman->lives <= 0) {
                    // Signal game over (main thread will handle menu state)
                    data->gameOver = true;
                    std::cout << "GAME OVER!" << std::endl;
                } else {
                    // Reset positions
                    data->pacman->spawn(*data->maze);
                    for (int j = 0; j < 4; j++) {
                        data->ghosts[j]->spawn(*data->maze, data->maze->getGhostSpawnNode(j));
                        data->ghosts[j]->state = GhostState::CHASE;
                    }
                }
                break;  // Don't check more collisions this frame
            }
        }
        
        // Check win condition
        if (data->maze->allCoinsCollected()) {
            data->gameWon = true;
            std::cout << "YOU WIN! Score: " << data->pacman->score << std::endl;
        }
        
        pthread_mutex_unlock(data->gameMutex);
        
        usleep(16000);  // ~60 FPS
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
    
    // Ghost House - Need KEY then EXIT_PERMIT (deadlock prevention via ordering)
    // All ghosts acquire in same order: key first, then permit
    std::cout << "Ghost " << data->ghostIndex << " waiting for key..." << std::endl;
    sem_wait(data->keySem);  // Acquire key first
    std::cout << "Ghost " << data->ghostIndex << " got key, waiting for exit permit..." << std::endl;
    sem_wait(data->exitPermitSem);  // Then acquire permit
    
    if (!gamePaused.load()) {
        std::cout << "Ghost " << data->ghostIndex << " leaving house (has key + permit)" << std::endl;
        data->ghost->leaveHouse();
    }
    
    // Release in reverse order (best practice, though doesn't affect deadlock)
    sem_post(data->exitPermitSem);
    sem_post(data->keySem);
    std::cout << "Ghost " << data->ghostIndex << " released key and permit" << std::endl;
    
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
            
            // Ghost House - Need KEY then EXIT_PERMIT for revive too
            sem_wait(data->keySem);
            sem_wait(data->exitPermitSem);
            
            pthread_mutex_lock(data->gameMutex);
            if (data->ghost->state == GhostState::IN_HOUSE && !data->pacman->powered) {
                std::cout << "Ghost " << data->ghostIndex << " reviving (key + permit)" << std::endl;
                data->ghost->leaveHouse();
            }
            pthread_mutex_unlock(data->gameMutex);
            
            sem_post(data->exitPermitSem);
            sem_post(data->keySem);
        } else {
            pthread_mutex_unlock(data->gameMutex);
            
            // Speed boost with PRIORITY for faster ghosts
            // Blinky (0) and Pinky (1) are "faster" - they try more often
            static int boostCounter[4] = {0, 0, 0, 0};
            int tryChance = (data->ghostIndex < 2) ? 100 : 300;  // Faster ghosts: 1/100, slower: 1/300
            
            if (boostCounter[data->ghostIndex] > 0) {
                // Ghost currently has speed boost active
                boostCounter[data->ghostIndex]--;
                pthread_mutex_lock(data->gameMutex);
                data->ghost->speed = data->ghost->baseSpeed * 2.0f;  // 2x speed boost!
                pthread_mutex_unlock(data->gameMutex);
                
                if (boostCounter[data->ghostIndex] == 0) {
                    // Boost expired, release semaphore
                    std::cout << "Ghost " << data->ghostIndex << " speed boost ended" << std::endl;
                    sem_post(data->speedBoost);
                }
            } else if (rand() % tryChance == 0) {
                // Try to acquire speed boost (2 available)
                if (sem_trywait(data->speedBoost) == 0) {
                    std::cout << "Ghost " << data->ghostIndex << " got speed boost!" << std::endl;
                    boostCounter[data->ghostIndex] = 150;  // ~3 seconds at 50 FPS
                }
            }
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
    changeToExecutableDir();
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
    threadManager.setupPacmanThread(&maze, &pacman);
    for (int i = 0; i < 4; i++) {
        threadManager.setupGhostThread(i, &maze, &pacman, ghosts[i]);
    }
    
    // Create threads
    pthread_create(&threadManager.gameThread, nullptr, gameEngineThread, &threadManager.gameData);
    pthread_create(&threadManager.pacmanThread, nullptr, pacmanControllerThread, &threadManager.pacmanData);
    for (int i = 0; i < 4; i++) {
        pthread_create(&threadManager.ghostThreads[i], nullptr, ghostControllerThread, &threadManager.ghostData[i]);
    }
    
    std::cout << "Started 6 threads (1 engine + 1 Pac-Man + 4 ghosts)" << std::endl;
    
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
            gamePaused.store(false);  // Unpauses all worker threads
            menu.hasActiveGame = true;  // Mark that game is in progress
            
            // Signal threads that game has started
            if (!gameStarted.load()) {
                gameStarted.store(true);
            }
            
            // Check for game over/win from game engine thread
            if (threadManager.gameData.gameOver) {
                menu.state = MenuState::GAME_OVER;
                threadManager.gameData.gameOver = false;  // Reset flag
            }
            if (threadManager.gameData.gameWon) {
                menu.state = MenuState::WIN;
                threadManager.gameData.gameWon = false;  // Reset flag
            }
        } else {
            // Game not playing - pause all worker threads
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
