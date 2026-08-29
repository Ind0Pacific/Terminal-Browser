#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <optional>

class WindowManager {
public:
    static void open_browser_window(const std::string& page_title, const std::string& page_text) {
        sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "My C++ Browser - " + page_title);
        sf::Font font;
        
      
        if (!font.openFromFile("arial.ttf")) {
            return;
        }
        sf::Text h1_text(font, page_text, 20);
        h1_text.setFillColor(sf::Color::Black);

        h1_text.setPosition(sf::Vector2f(50.f, 50.f));

        sf::RectangleShape div_box(sf::Vector2f(700.f, 400.f));
        div_box.setFillColor(sf::Color(240, 240, 240)); 
        div_box.setOutlineColor(sf::Color(200, 200, 200));
        div_box.setOutlineThickness(2.f);
        div_box.setPosition(sf::Vector2f(40.f, 40.f));

        while (window.isOpen()) {
            while (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }
            
            window.clear(sf::Color::White);
            window.draw(div_box);
            window.draw(h1_text);
            window.display();
        }
    }
};

#endif