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

    static std::string open_browser_window(const std::string& current_url, const std::vector<DOMNode>& dom) {
        sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "My C++ Browser");
        sf::Font font;
        if (!font.openFromFile("arial.ttf")) return "";

        sf::RectangleShape top_bar(sf::Vector2f(800.f, 60.f));
        top_bar.setFillColor(sf::Color(200, 200, 200));

        sf::RectangleShape address_box(sf::Vector2f(700.f, 40.f));
        address_box.setFillColor(sf::Color::White);
        address_box.setOutlineColor(sf::Color(150, 150, 150));
        address_box.setOutlineThickness(2.f);
        address_box.setPosition(sf::Vector2f(50.f, 10.f));

        std::string input_string = current_url;
        sf::Text address_text(font, input_string, 20);
        address_text.setFillColor(sf::Color::Black);
        address_text.setPosition(sf::Vector2f(60.f, 15.f));

        bool is_typing = false;
        std::string return_action = "";

        std::vector<sf::Text> render_tree;
        std::vector<Hitbox> link_hitboxes; 
        float current_y = 90.f; 

        for (const auto& node : dom) {
            unsigned int size = 16;
            sf::Color color = sf::Color::Black;

            if (node.tag == "h1" || node.tag == "h2") {
                size = 24; color = sf::Color(50, 50, 150); 
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

            if (node.tag == "a" && !node.link_url.empty()) {
                link_hitboxes.push_back({ui_text.getGlobalBounds(), node.link_url});
            }
            current_y += ui_text.getLocalBounds().size.y + 15.f; 
        }

        float total_height = std::max(400.f, current_y - 30.f);
        sf::RectangleShape div_box(sf::Vector2f(700.f, total_height));
        div_box.setFillColor(sf::Color(240, 240, 240)); 
        div_box.setPosition(sf::Vector2f(50.f, 70.f));

        sf::View ui_view = window.getDefaultView(); 
        sf::View dom_view = window.getDefaultView(); 
        float scroll_y = dom_view.getCenter().y;
        float min_scroll = scroll_y; 
        float max_scroll = std::max(min_scroll, total_height + 150.f - (window.getSize().y / 2.f));

        while (window.isOpen()) {
            while (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } 
                else if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                        scroll_y -= scrollEvent->delta * 40.f; 
                        scroll_y = std::clamp(scroll_y, min_scroll, max_scroll);
                        dom_view.setCenter(sf::Vector2f(dom_view.getCenter().x, scroll_y));
                    }
                }
                else if (const auto* clickEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (clickEvent->button == sf::Mouse::Button::Left) {
                        sf::Vector2f static_mouse_pos(clickEvent->position.x, clickEvent->position.y);
                        
                        if (address_box.getGlobalBounds().contains(static_mouse_pos)) {
                            is_typing = true;
                            address_box.setOutlineColor(sf::Color::Blue);
                        } else {
                            is_typing = false;
                            address_box.setOutlineColor(sf::Color(150, 150, 150));
                            
                            sf::Vector2f world_mouse_pos = window.mapPixelToCoords(
                                sf::Vector2i(clickEvent->position.x, clickEvent->position.y), dom_view);
                            
                            for (const auto& box : link_hitboxes) {
                                if (box.bounds.contains(world_mouse_pos)) {
                                    return_action = "LINK:" + box.url;
                                    window.close();
                                }
                            }
                        }
                    }
                }
                else if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                    if (is_typing) {
                        if (textEvent->unicode == '\b' && !input_string.empty()) { 
                            input_string.pop_back();
                        } 
                        else if (textEvent->unicode == 13) { 
                            return_action = "NEW_DOMAIN:" + input_string;
                            window.close();
                        }
                        else if (textEvent->unicode >= 32 && textEvent->unicode < 127) { 
                            input_string += static_cast<char>(textEvent->unicode);
                        }
                        address_text.setString(input_string);
                    }
                }
            }
            
            window.clear(sf::Color::White);
            
            window.setView(dom_view); 
            window.draw(div_box);
            for (const auto& t : render_tree) window.draw(t);
            
            window.setView(ui_view);
            window.draw(top_bar);
            window.draw(address_box);
            window.draw(address_text);

            window.display();
        }
        return return_action;
    }
};

#endif