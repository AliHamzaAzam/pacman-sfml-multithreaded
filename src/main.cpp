#include "Constants.h"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
#ifdef __APPLE__
    setenv("SFML_SILENCE_MACOS_KEYBOARD_WARNING", "1", 1);
#endif

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT)),
        "Pac-Man"
    );
    window.setFramerateLimit(Config::FPS);

    std::cout << "Pac-Man initialized successfully!" << std::endl;
    std::cout << "Window: " << Config::WINDOW_WIDTH << "x" << Config::WINDOW_HEIGHT << std::endl;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);
        window.display();
    }

    std::cout << "Pac-Man closed cleanly." << std::endl;
    return 0;
}
