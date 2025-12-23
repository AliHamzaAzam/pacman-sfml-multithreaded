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
    sf::Font font;
    bool fontLoaded;
    
    Menu() : state(MenuState::MAIN_MENU), selectedItem(0), fontLoaded(false) {
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
                if (key == sf::Keyboard::Key::Up) {
                    selectedItem = (selectedItem - 1 + 2) % 2;
                }
                else if (key == sf::Keyboard::Key::Down) {
                    selectedItem = (selectedItem + 1) % 2;
                }
                else if (key == sf::Keyboard::Key::Enter) {
                    if (selectedItem == 0) {
                        state = MenuState::PLAYING;
                    }
                    // selectedItem == 1 means Exit, handled in main
                }
                break;
                
            case MenuState::PLAYING:
                if (key == sf::Keyboard::Key::P || key == sf::Keyboard::Key::Escape) {
                    state = MenuState::PAUSED;
                }
                break;
                
            case MenuState::PAUSED:
                if (key == sf::Keyboard::Key::P || key == sf::Keyboard::Key::Escape) {
                    state = MenuState::PLAYING;
                }
                break;
                
            case MenuState::GAME_OVER:
            case MenuState::WIN:
                if (key == sf::Keyboard::Key::Enter) {
                    state = MenuState::MAIN_MENU;
                    selectedItem = 0;
                }
                break;
        }
    }
    
    bool shouldExit() const {
        return state == MenuState::MAIN_MENU && selectedItem == 1;
    }
    
private:
    void drawMainMenu(sf::RenderWindow& window) {
        // Darken background
        sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);
        
        // Title
        sf::Text title(font);
        title.setString("PAC-MAN");
        title.setCharacterSize(64);
        title.setFillColor(sf::Color::Yellow);
        title.setStyle(sf::Text::Bold);
        sf::FloatRect titleBounds = title.getLocalBounds();
        title.setOrigin(sf::Vector2f(titleBounds.size.x / 2, titleBounds.size.y / 2));
        title.setPosition(sf::Vector2f(window.getSize().x / 2.f, 150.f));
        window.draw(title);
        
        // Menu items
        const char* items[] = {"START GAME", "EXIT"};
        for (int i = 0; i < 2; i++) {
            sf::Text item(font);
            item.setString(items[i]);
            item.setCharacterSize(36);
            item.setFillColor(i == selectedItem ? sf::Color::Yellow : sf::Color::White);
            sf::FloatRect itemBounds = item.getLocalBounds();
            item.setOrigin(sf::Vector2f(itemBounds.size.x / 2, itemBounds.size.y / 2));
            item.setPosition(sf::Vector2f(window.getSize().x / 2.f, 300.f + i * 60.f));
            window.draw(item);
        }
        
        // Instructions
        sf::Text instructions(font);
        instructions.setString("Use Arrow Keys + Enter");
        instructions.setCharacterSize(20);
        instructions.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect instrBounds = instructions.getLocalBounds();
        instructions.setOrigin(sf::Vector2f(instrBounds.size.x / 2, instrBounds.size.y / 2));
        instructions.setPosition(sf::Vector2f(window.getSize().x / 2.f, 500.f));
        window.draw(instructions);
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
        text.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f - 30));
        window.draw(text);
        
        sf::Text sub(font);
        sub.setString("Press P to Resume");
        sub.setCharacterSize(24);
        sub.setFillColor(sf::Color::White);
        sf::FloatRect subBounds = sub.getLocalBounds();
        sub.setOrigin(sf::Vector2f(subBounds.size.x / 2, subBounds.size.y / 2));
        sub.setPosition(sf::Vector2f(window.getSize().x / 2.f, window.getSize().y / 2.f + 30));
        window.draw(sub);
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
        sub.setString("Press Enter to Continue");
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
        sub.setString("Press Enter to Continue");
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
