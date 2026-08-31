#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <optional>
#include <sstream>
#include <vector>
#include <algorithm>
#include "htmlParser.hpp" 

class WindowManager {
public:
    static std::string wrapText(const std::string& text, const sf::Font& font, unsigned int characterSize, float maxWidth) {
        std::istringstream words(text);
        std::string word, wrappedText = "", currentLine = "";
        sf::Text measureText(font, "", characterSize);

        while (words >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            measureText.setString(testLine);
            if (measureText.getLocalBounds().size.x > maxWidth) {
                wrappedText += currentLine + "\n";
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
        return wrappedText + currentLine;
    }

    static void open_browser_window(const std::string& page_title, const std::vector<DOMNode>& dom) {
        sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "My C++ Browser - " + page_title);
        sf::Font font;
        if (!font.openFromFile("arial.ttf")) return;

        std::vector<sf::Text> render_tree;
        float current_y = 70.f;

        for (const auto& node : dom) {
            unsigned int size = 16;
            sf::Color color = sf::Color::Black;

            if (node.tag == "h1" || node.tag == "h2") {
                size = 24;
                color = sf::Color(50, 50, 150); 
            } else if (node.tag == "a") {
                color = sf::Color::Blue; 
            } else if (node.tag == "b" || node.tag == "strong") {
                color = sf::Color(150, 0, 0); 
            }

            std::string formatted_text = wrapText(node.text, font, size, 660.f);
            sf::Text ui_text(font, formatted_text, size);
            ui_text.setFillColor(color);
            ui_text.setPosition(sf::Vector2f(70.f, current_y));
            
            render_tree.push_back(ui_text);
            current_y += ui_text.getLocalBounds().size.y + 15.f; 
        }

        float total_height = std::max(400.f, current_y - 30.f);
        sf::RectangleShape div_box(sf::Vector2f(700.f, total_height));
        div_box.setFillColor(sf::Color(240, 240, 240)); 
        div_box.setOutlineColor(sf::Color(200, 200, 200));
        div_box.setOutlineThickness(2.f);
        div_box.setPosition(sf::Vector2f(50.f, 50.f));

        // Initialize the Scrolling Camera
        sf::View view = window.getDefaultView();
        float scroll_y = view.getCenter().y;
        float min_scroll = scroll_y; // Cannot scroll above the top
        float max_scroll = std::max(min_scroll, total_height + 100.f - (window.getSize().y / 2.f));

        while (window.isOpen()) {
            while (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } 
                // Listen for Mouse Wheel events
                else if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                        // Move camera based on scroll delta
                        scroll_y -= scrollEvent->delta * 40.f; 
                        // Clamp the camera so it doesn't leave the document bounds
                        scroll_y = std::clamp(scroll_y, min_scroll, max_scroll);
                        view.setCenter(sf::Vector2f(view.getCenter().x, scroll_y));
                    }
                }
            }
            
            window.setView(view); // Apply the camera view to the window
            window.clear(sf::Color::White);
            window.draw(div_box);
            for (const auto& t : render_tree) {
                window.draw(t);
            }
            window.display();
        }
    }
};

#endif