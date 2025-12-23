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
    Maze* maze;
    Pacman* pacman;
    Ghost** ghosts;
    int numGhosts;
    bool running;
    
    GameThreadData() : gameMutex(nullptr), maze(nullptr), pacman(nullptr),
                       ghosts(nullptr), numGhosts(0), running(true) {}
};

// Data structure passed to each ghost thread
struct GhostThreadData {
    pthread_mutex_t* gameMutex;
    Maze* maze;
    Pacman* pacman;
    Ghost* ghost;
    int ghostIndex;
    sem_t* spawnSemaphore;    // Controls ghost exit from house
    sem_t* speedBoost;        // Power pellet synchronization
    bool running;
    
    GhostThreadData() : gameMutex(nullptr), maze(nullptr), pacman(nullptr),
                        ghost(nullptr), ghostIndex(0), spawnSemaphore(nullptr),
                        speedBoost(nullptr), running(true) {}
};

class ThreadManager {
public:
    // Mutexes
    pthread_mutex_t gameMutex;
    
    // Semaphores 
    sem_t* spawnSemaphore;    // Controls ghost spawning rate
    sem_t* speedBoost;        // Power pellet effect
    
    // Thread handles
    pthread_t gameThread;
    pthread_t ghostThreads[4];
    
    // Thread data
    GameThreadData gameData;
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
        
        // Create semaphores
        // spawnSemaphore: starts at 1, allows ghosts to leave one at a time
        spawnSemaphore = sem_open("/pacman_spawn", O_CREAT | O_EXCL, 0644, 1);
        if (spawnSemaphore == SEM_FAILED) {
            std::cerr << "Failed to create spawn semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        // speedBoost: starts at 0, signaled when Pac-Man gets power pellet
        speedBoost = sem_open("/pacman_boost", O_CREAT | O_EXCL, 0644, 0);
        if (speedBoost == SEM_FAILED) {
            std::cerr << "Failed to create speed boost semaphore: " << strerror(errno) << std::endl;
            return false;
        }
        
        std::cout << "ThreadManager: Initialized mutex and semaphores" << std::endl;
        return true;
    }
    
    void setupGameThread(Maze* maze, Pacman* pacman, Ghost** ghosts, int numGhosts) {
        gameData.gameMutex = &gameMutex;
        gameData.maze = maze;
        gameData.pacman = pacman;
        gameData.ghosts = ghosts;
        gameData.numGhosts = numGhosts;
        gameData.running = true;
    }
    
    void setupGhostThread(int index, Maze* maze, Pacman* pacman, Ghost* ghost) {
        ghostData[index].gameMutex = &gameMutex;
        ghostData[index].maze = maze;
        ghostData[index].pacman = pacman;
        ghostData[index].ghost = ghost;
        ghostData[index].ghostIndex = index;
        ghostData[index].spawnSemaphore = spawnSemaphore;
        ghostData[index].speedBoost = speedBoost;
        ghostData[index].running = true;
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
