#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <optional>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include "htmlParser.hpp" 

struct Hitbox {
    sf::FloatRect bounds;
    std::string url;
};

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

    static std::string open_browser_window(const std::string& page_title, const std::vector<DOMNode>& dom) {
        sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "My C++ Browser - " + page_title);
        sf::Font font;
        if (!font.openFromFile("arial.ttf")) return "";

        std::vector<sf::Text> render_tree;
        std::vector<Hitbox> link_hitboxes; // Array to hold mathematical bounds
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

            // 3. Record the Hitbox if the node is a link
            if (node.tag == "a" && !node.link_url.empty()) {
                link_hitboxes.push_back({ui_text.getGlobalBounds(), node.link_url});
            }

            current_y += ui_text.getLocalBounds().size.y + 15.f; 
        }

        float total_height = std::max(400.f, current_y - 30.f);
        sf::RectangleShape div_box(sf::Vector2f(700.f, total_height));
        div_box.setFillColor(sf::Color(240, 240, 240)); 
        div_box.setOutlineColor(sf::Color(200, 200, 200));
        div_box.setOutlineThickness(2.f);
        div_box.setPosition(sf::Vector2f(50.f, 50.f));

        sf::View view = window.getDefaultView();
        float scroll_y = view.getCenter().y;
        float min_scroll = scroll_y; 
        float max_scroll = std::max(min_scroll, total_height + 100.f - (window.getSize().y / 2.f));

        std::string clicked_url = "";

        while (window.isOpen()) {
            while (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } 
                else if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                        scroll_y -= scrollEvent->delta * 40.f; 
                        scroll_y = std::clamp(scroll_y, min_scroll, max_scroll);
                        view.setCenter(sf::Vector2f(view.getCenter().x, scroll_y));
                    }
                }
                //Mouse Click Detection
                else if (const auto* clickEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (clickEvent->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mouse_world_pos = window.mapPixelToCoords(
                            sf::Vector2i(clickEvent->position.x, clickEvent->position.y), view);
                        
                        for (const auto& box : link_hitboxes) {
                            if (box.bounds.contains(mouse_world_pos)) {
                                std::cout << "[CLICK DETECTED] Intersected with link: " << box.url << std::endl;
                                clicked_url = box.url;
                                window.close(); // Close GUI next page load
                            }
                        }
                    }
                }
            }
            
            window.setView(view); 
            window.clear(sf::Color::White);
            window.draw(div_box);
            for (const auto& t : render_tree) {
                window.draw(t);
            }
            window.display();
        }
        return clicked_url;
    }
};

#endif