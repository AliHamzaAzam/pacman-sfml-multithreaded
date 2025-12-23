#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Config {
    // Window settings - sized to fit maze + HUD
    constexpr int WINDOW_WIDTH = 650;   // Maze scaled width + margins
    constexpr int WINDOW_HEIGHT = 750;  // Maze scaled height + HUD area
    constexpr int FPS = 60;
    
    // Maze settings
    constexpr int MAZE_ROWS = 35;
    constexpr int MAZE_COLS = 33;
    constexpr int CELL_WIDTH = 21;
    constexpr int CELL_HEIGHT = 23;
    constexpr int MAZE_OFFSET_X = 5;
    constexpr int MAZE_OFFSET_Y = 130;
    
    // Game settings
    constexpr int INITIAL_LIVES = 3;
    constexpr int COIN_SCORE = 10;
    constexpr int POWER_PELLET_DURATION = 200;
    
    // Direction constants
    constexpr int DIR_NONE = 0;
    constexpr int DIR_RIGHT = 1;
    constexpr int DIR_DOWN = 2;
    constexpr int DIR_LEFT = 3;
    constexpr int DIR_UP = 4;
    
    // Game states
    constexpr char STATE_MENU = 'M';
    constexpr char STATE_PLAYING = 'P';
    constexpr char STATE_PAUSED = 'A';
    constexpr char STATE_GAMEOVER = 'G';
    
    // Resource paths
    constexpr const char* SPRITE_SHEET = "resources/sprites.png";
    constexpr const char* FONT_PACMAN = "resources/PAC-FONT.TTF";
    constexpr const char* FONT_CRACKMAN = "resources/Crackman.otf";
}

#endif // CONSTANTS_H
