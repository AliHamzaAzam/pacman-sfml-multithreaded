#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

// Forward declarations
class Maze;
class Pacman;
class Ghost;

// Data structure passed to game engine thread
struct GameThreadData {
    pthread_mutex_t* gameMutex;
    pthread_rwlock_t* boardLock;  // Reader-writer lock 
    Maze* maze;
    Pacman* pacman;
    Ghost** ghosts;
    int numGhosts;
    bool running;
    bool gameOver;   // Signal game over to main thread
    bool gameWon;    // Signal win to main thread
    
    GameThreadData() : gameMutex(nullptr), boardLock(nullptr), maze(nullptr), 
                       pacman(nullptr), ghosts(nullptr), numGhosts(0), running(true),
                       gameOver(false), gameWon(false) {}
};

// Data structure passed to each ghost thread
struct GhostThreadData {
    pthread_mutex_t* gameMutex;
    pthread_rwlock_t* boardLock;  // Reader-writer lock 
    Maze* maze;
    Pacman* pacman;
    Ghost* ghost;
    int ghostIndex;
    sem_t* spawnSemaphore;    // Controls ghost exit from house
    sem_t* speedBoost;        // Speed boost 
    sem_t* keySem;            // Ghost house key 
    sem_t* exitPermitSem;     // Ghost house exit permit 
    bool running;
    
    GhostThreadData() : gameMutex(nullptr), boardLock(nullptr), maze(nullptr), 
                        pacman(nullptr), ghost(nullptr), ghostIndex(0), 
                        spawnSemaphore(nullptr), speedBoost(nullptr),
                        keySem(nullptr), exitPermitSem(nullptr), running(true) {}
};

// Pac-Man thread data structure
struct PacManThreadData {
    pthread_mutex_t* gameMutex;
    pthread_rwlock_t* boardLock;  // Reader-writer lock 
    Maze* maze;
    Pacman* pacman;
    sem_t* powerPelletSem;        // Power pellet control 
    bool running;
    
    PacManThreadData() : gameMutex(nullptr), boardLock(nullptr), maze(nullptr), 
                         pacman(nullptr), powerPelletSem(nullptr), running(true) {}
};

class ThreadManager {
public:
    // Mutexes
    pthread_mutex_t gameMutex;
    
    // Reader-Writer Lock
    pthread_rwlock_t boardLock;
    
    // Semaphores
    sem_t* spawnSemaphore;    // Controls ghost spawning rate
    sem_t* speedBoost;        // Speed boost 
    sem_t* keySem;            // Ghost house key 
    sem_t* exitPermitSem;     // Ghost house exit permit 
    sem_t* powerPelletSem;    // Power pellet eating 
    
    // Thread handles
    pthread_t gameThread;     // Game engine (collisions, scoring)
    pthread_t pacmanThread;   // Pac-Man movement
    pthread_t ghostThreads[4];
    
    // Thread data
    GameThreadData gameData;
    PacManThreadData pacmanData;
    GhostThreadData ghostData[4];

    
    ThreadManager() : spawnSemaphore(SEM_FAILED), speedBoost(SEM_FAILED) {}
    
    bool initialize() {
        // Initialize mutex
        if (pthread_mutex_init(&gameMutex, nullptr) != 0) {
            std::cerr << "Failed to initialize game mutex" << std::endl;
            return false;
        }
            
        // Clean up any leftover semaphores
        sem_unlink("/pacman_spawn");
        sem_unlink("/pacman_boost");
        sem_unlink("/pacman_key");
        sem_unlink("/pacman_permit");
        sem_unlink("/pacman_pellet");
        
        // Initialize reader-writer lock
        if (pthread_rwlock_init(&boardLock, nullptr) != 0) {
            std::cerr << "Failed to initialize board rwlock" << std::endl;
            return false;
        }
        
        // Create semaphores
        // spawnSemaphore: starts at 1, allows ghosts to leave one at a time
        spawnSemaphore = sem_open("/pacman_spawn", O_CREAT | O_EXCL, 0644, 1);
        if (spawnSemaphore == SEM_FAILED) {
            std::cerr << "Failed to create spawn semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        // speedBoost: starts at 2, two ghosts can have speed boost 
        speedBoost = sem_open("/pacman_boost", O_CREAT | O_EXCL, 0644, 2);
        if (speedBoost == SEM_FAILED) {
            std::cerr << "Failed to create speed boost semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        // keySem: Ghost house key, starts at 1
        keySem = sem_open("/pacman_key", O_CREAT | O_EXCL, 0644, 1);
        if (keySem == SEM_FAILED) {
            std::cerr << "Failed to create key semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        // exitPermitSem: Ghost house exit permit, starts at 1 (Scenario 3)
        exitPermitSem = sem_open("/pacman_permit", O_CREAT | O_EXCL, 0644, 1);
        if (exitPermitSem == SEM_FAILED) {
            std::cerr << "Failed to create exit permit semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        // powerPelletSem: Power pellet control, starts at 4 (Scenario 2)
        powerPelletSem = sem_open("/pacman_pellet", O_CREAT | O_EXCL, 0644, 4);
        if (powerPelletSem == SEM_FAILED) {
            std::cerr << "Failed to create power pellet semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        std::cout << "ThreadManager: Initialized mutex, rwlock, and 5 semaphores" << std::endl;
        return true;
    }
    
    void setupGameThread(Maze* maze, Pacman* pacman, Ghost** ghosts, int numGhosts) {
        gameData.gameMutex = &gameMutex;
        gameData.boardLock = &boardLock;
        gameData.maze = maze;
        gameData.pacman = pacman;
        gameData.ghosts = ghosts;
        gameData.numGhosts = numGhosts;
        gameData.running = true;
    }
    
    void setupGhostThread(int index, Maze* maze, Pacman* pacman, Ghost* ghost) {
        ghostData[index].gameMutex = &gameMutex;
        ghostData[index].boardLock = &boardLock;
        ghostData[index].maze = maze;
        ghostData[index].pacman = pacman;
        ghostData[index].ghost = ghost;
        ghostData[index].ghostIndex = index;
        ghostData[index].spawnSemaphore = spawnSemaphore;
        ghostData[index].speedBoost = speedBoost;
        ghostData[index].keySem = keySem;
        ghostData[index].exitPermitSem = exitPermitSem;
        ghostData[index].running = true;
    }
    
    void setupPacmanThread(Maze* maze, Pacman* pacman) {
        pacmanData.gameMutex = &gameMutex;
        pacmanData.boardLock = &boardLock;
        pacmanData.maze = maze;
        pacmanData.pacman = pacman;
        pacmanData.powerPelletSem = powerPelletSem;
        pacmanData.running = true;
    }
    
    void signalPowerUp() {
        // Signal all waiting ghost threads that Pac-Man got a power pellet
        for (int i = 0; i < 4; i++) {
            sem_post(speedBoost);
        }
    }
    
    void stopAll() {
        gameData.running = false;
        for (int i = 0; i < 4; i++) {
            ghostData[i].running = false;
        }
    }
    
    void cleanup() {
        // Destroy mutex
        pthread_mutex_destroy(&gameMutex);
        
        // Close and unlink semaphores
        if (spawnSemaphore != SEM_FAILED) {
            sem_close(spawnSemaphore);
            sem_unlink("/pacman_spawn");
        }
        if (speedBoost != SEM_FAILED) {
            sem_close(speedBoost);
            sem_unlink("/pacman_boost");
        }
        
        std::cout << "ThreadManager: Cleaned up mutex and semaphores" << std::endl;
    }
    
    ~ThreadManager() {
        cleanup();
    }
};

#endif // THREAD_MANAGER_H
