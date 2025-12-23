#ifndef MENU_H
#define MENU_H

#include <SFML/Graphics.hpp>
#include <string>

enum class MenuState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

class Menu {
public:
    MenuState state;
    int selectedItem;
    int pauseSelectedItem;  // For pause menu navigation
    sf::Font font;
    bool fontLoaded;
    bool hasActiveGame;     // Track if there's a game to continue
    bool requestNewGame;    // Signal to reset game
    bool requestExitToMenu; // Signal to exit to main menu
    
    Menu() : state(MenuState::MAIN_MENU), selectedItem(0), pauseSelectedItem(0),
             fontLoaded(false), hasActiveGame(false), requestNewGame(false), requestExitToMenu(false) {
        if (font.openFromFile("resources/Crackman.otf")) {
            fontLoaded = true;
        }
    }
    
    void draw(sf::RenderWindow& window, int score, int lives) {
        if (!fontLoaded) return;
        
        switch (state) {
            case MenuState::MAIN_MENU:
                drawMainMenu(window);
                break;
            case MenuState::PAUSED:
                drawPaused(window);
                break;
            case MenuState::GAME_OVER:
                drawGameOver(window, score);
                break;
            case MenuState::WIN:
                drawWin(window, score);
                break;
            case MenuState::PLAYING:
                drawHUD(window, score, lives);
                break;
        }
    }
    
    void handleInput(sf::Keyboard::Key key) {
        switch (state) {
            case MenuState::MAIN_MENU:
                handleMainMenuInput(key);
                break;
            case MenuState::PLAYING:
                if (key == sf::Keyboard::Key::P || key == sf::Keyboard::Key::Escape) {
                    state = MenuState::PAUSED;
                    pauseSelectedItem = 0;
                }
                break;
            case MenuState::PAUSED:
                handlePauseInput(key);
                break;
            case MenuState::GAME_OVER:
            case MenuState::WIN:
                if (key == sf::Keyboard::Key::Enter) {
                    state = MenuState::MAIN_MENU;
                    selectedItem = 0;
                    hasActiveGame = false;
                }
                break;
        }
    }
    
    bool shouldExit() const {
        // EXIT is item 2 when hasActiveGame, item 1 otherwise
        int exitItem = hasActiveGame ? 2 : 1;
        return state == MenuState::MAIN_MENU && selectedItem == exitItem;
    }
    
private:
    void handleMainMenuInput(sf::Keyboard::Key key) {
        int menuItems = hasActiveGame ? 3 : 2;  // 3 items if game active: Continue, New Game, Exit
        
        if (key == sf::Keyboard::Key::Up) {
            selectedItem = (selectedItem - 1 + menuItems) % menuItems;
        }
        else if (key == sf::Keyboard::Key::Down) {
            selectedItem = (selectedItem + 1) % menuItems;
        }
        else if (key == sf::Keyboard::Key::Enter) {
            if (hasActiveGame) {
                // Continue, New Game, Exit
                if (selectedItem == 0) {
                    state = MenuState::PLAYING;  // Continue
                } else if (selectedItem == 1) {
                    requestNewGame = true;       // New Game
                    state = MenuState::PLAYING;
                }
                // selectedItem == 2 is Exit, handled in main
            } else {
                // New Game, Exit
                if (selectedItem == 0) {
                    requestNewGame = true;
                    state = MenuState::PLAYING;
                }
                // selectedItem == 1 is Exit, handled in main
            }
        }
    }
    
    void handlePauseInput(sf::Keyboard::Key key) {
        if (key == sf::Keyboard::Key::Up) {
            pauseSelectedItem = (pauseSelectedItem - 1 + 2) % 2;
        }
        else if (key == sf::Keyboard::Key::Down) {
            pauseSelectedItem = (pauseSelectedItem + 1) % 2;
        }
        else if (key == sf::Keyboard::Key::P || key == sf::Keyboard::Key::Escape) {
            state = MenuState::PLAYING;
        }
        else if (key == sf::Keyboard::Key::Enter) {
            if (pauseSelectedItem == 0) {
                state = MenuState::PLAYING;  // Resume
            } else {
                requestExitToMenu = true;    // Exit to Main Menu
                state = MenuState::MAIN_MENU;
                selectedItem = 0;
            }
        }
    }
    
    void drawMainMenu(sf::RenderWindow& window) {
        float centerX = window.getSize().x / 2.f;
        float centerY = window.getSize().y / 2.f;
        
        // Gradient-like dark overlay
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);
        
        // Decorative top bar
        sf::RectangleShape topBar(sf::Vector2f(400.f, 4.f));
        topBar.setFillColor(sf::Color::Yellow);
        topBar.setOrigin(sf::Vector2f(200.f, 2.f));
        topBar.setPosition(sf::Vector2f(centerX, 80.f));
        window.draw(topBar);
        
        // Title with shadow effect
        sf::Text titleShadow(font);
        titleShadow.setString("PAC-MAN");
        titleShadow.setCharacterSize(72);
        titleShadow.setFillColor(sf::Color(50, 50, 0));
        sf::FloatRect shadowBounds = titleShadow.getLocalBounds();
        titleShadow.setOrigin(sf::Vector2f(shadowBounds.size.x / 2, shadowBounds.size.y / 2));
        titleShadow.setPosition(sf::Vector2f(centerX + 3, 135.f));
        window.draw(titleShadow);
        
        sf::Text title(font);
        title.setString("PAC-MAN");
        title.setCharacterSize(72);
        title.setFillColor(sf::Color::Yellow);
        sf::FloatRect titleBounds = title.getLocalBounds();
        title.setOrigin(sf::Vector2f(titleBounds.size.x / 2, titleBounds.size.y / 2));
        title.setPosition(sf::Vector2f(centerX, 130.f));
        window.draw(title);
        
        // Decorative bottom bar under title
        sf::RectangleShape bottomBar(sf::Vector2f(400.f, 4.f));
        bottomBar.setFillColor(sf::Color::Yellow);
        bottomBar.setOrigin(sf::Vector2f(200.f, 2.f));
        bottomBar.setPosition(sf::Vector2f(centerX, 215.f));
        window.draw(bottomBar);
        
        // Menu items - centered in screen
        float startY = centerY - 30.f;
        float spacing = 60.f;
        
        if (hasActiveGame) {
            const char* items[] = {"CONTINUE", "NEW GAME", "EXIT"};
            for (int i = 0; i < 3; i++) {
                drawMenuItem(window, items[i], startY + i * spacing, i == selectedItem);
            }
        } else {
            const char* items[] = {"NEW GAME", "EXIT"};
            for (int i = 0; i < 2; i++) {
                drawMenuItem(window, items[i], startY + i * spacing, i == selectedItem);
            }
        }
        
        // Instructions at bottom
        sf::Text instructions(font);
        instructions.setString("ARROW KEYS + ENTER");
        instructions.setCharacterSize(18);
        instructions.setFillColor(sf::Color(120, 120, 120));
        sf::FloatRect instrBounds = instructions.getLocalBounds();
        instructions.setOrigin(sf::Vector2f(instrBounds.size.x / 2, instrBounds.size.y / 2));
        instructions.setPosition(sf::Vector2f(centerX, window.getSize().y - 40.f));
        window.draw(instructions);
    }
    
    void drawMenuItem(sf::RenderWindow& window, const char* text, float y, bool selected) {
        float centerX = window.getSize().x / 2.f;
        
        // Selection indicator (arrow)
        if (selected) {
            sf::Text arrow(font);
            arrow.setString(">");
            arrow.setCharacterSize(28);
            arrow.setFillColor(sf::Color::Yellow);
            arrow.setPosition(sf::Vector2f(centerX - 120.f, y - 15.f));
            window.draw(arrow);
            
            sf::Text arrowRight(font);
            arrowRight.setString("<");
            arrowRight.setCharacterSize(28);
            arrowRight.setFillColor(sf::Color::Yellow);
            arrowRight.setPosition(sf::Vector2f(centerX + 110.f, y - 15.f));
            window.draw(arrowRight);
        }
        
        sf::Text item(font);
        item.setString(text);
        item.setCharacterSize(selected ? 30 : 24);
        item.setFillColor(selected ? sf::Color::Yellow : sf::Color(180, 180, 180));
        sf::FloatRect itemBounds = item.getLocalBounds();
        item.setOrigin(sf::Vector2f(itemBounds.size.x / 2, itemBounds.size.y / 2));
        item.setPosition(sf::Vector2f(centerX, y));
        window.draw(item);
    }
    
    void drawPaused(sf::RenderWindow& window) {
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        window.draw(overlay);
        
        sf::Text text(font);
        text.setString("PAUSED");
        text.setCharacterSize(48);
        text.setFillColor(sf::Color::Yellow);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
        text.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f - 80));
        window.draw(text);
        
        // Menu options
        const char* items[] = {"RESUME", "EXIT TO MENU"};
        for (int i = 0; i < 2; i++) {
            drawMenuItem(window, items[i], window.getSize().y / 2.f + i * 50.f, i == pauseSelectedItem);
        }
    }
    
    void drawGameOver(sf::RenderWindow& window, int score) {
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(100, 0, 0, 200));
        window.draw(overlay);
        
        sf::Text text(font);
        text.setString("GAME OVER");
        text.setCharacterSize(48);
        text.setFillColor(sf::Color::Red);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
        text.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f - 50));
        window.draw(text);
        
        sf::Text scoreText(font);
        scoreText.setString("Score: " + std::to_string(score));
        scoreText.setCharacterSize(32);
        scoreText.setFillColor(sf::Color::White);
        sf::FloatRect scoreBounds = scoreText.getLocalBounds();
        scoreText.setOrigin(sf::Vector2f(scoreBounds.size.x / 2, scoreBounds.size.y / 2));
        scoreText.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f + 20));
        window.draw(scoreText);
        
        sf::Text sub(font);
        sub.setString("Press Enter");
        sub.setCharacterSize(20);
        sub.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect subBounds = sub.getLocalBounds();
        sub.setOrigin(sf::Vector2f(subBounds.size.x / 2, subBounds.size.y / 2));
        sub.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f + 80));
        window.draw(sub);
    }
    
    void drawWin(sf::RenderWindow& window, int score) {
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(0, 100, 0, 200));
        window.draw(overlay);
        
        sf::Text text(font);
        text.setString("YOU WIN!");
        text.setCharacterSize(48);
        text.setFillColor(sf::Color::Green);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
        text.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f - 50));
        window.draw(text);
        
        sf::Text scoreText(font);
        scoreText.setString("Final Score: " + std::to_string(score));
        scoreText.setCharacterSize(32);
        scoreText.setFillColor(sf::Color::White);
        sf::FloatRect scoreBounds = scoreText.getLocalBounds();
        scoreText.setOrigin(sf::Vector2f(scoreBounds.size.x / 2, scoreBounds.size.y / 2));
        scoreText.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f + 20));
        window.draw(scoreText);
        
        sf::Text sub(font);
        sub.setString("Press Enter");
        sub.setCharacterSize(20);
        sub.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect subBounds = sub.getLocalBounds();
        sub.setOrigin(sf::Vector2f(subBounds.size.x / 2, subBounds.size.y / 2));
        sub.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f + 80));
        window.draw(sub);
    }
    
    void drawHUD(sf::RenderWindow& window, int score, int lives) {
        float bottomY = window.getSize().y - 35.f;
        
        // Score (bottom left)
        sf::Text scoreText(font);
        scoreText.setString("SCORE " + std::to_string(score));
        scoreText.setCharacterSize(18);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(sf::Vector2f(20.f, bottomY));
        window.draw(scoreText);
        
        // Lives (bottom right)
        sf::Text livesText(font);
        livesText.setString("LIVES " + std::to_string(lives));
        livesText.setCharacterSize(18);
        livesText.setFillColor(sf::Color::White);
        livesText.setPosition(sf::Vector2f(window.getSize().x - 110.f, bottomY));
        window.draw(livesText);
    }
};

#endif // MENU_H
