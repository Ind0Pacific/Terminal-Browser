#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <optional>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include "htmlParser.hpp" 

struct Hitbox { sf::FloatRect bounds; std::string url; };
struct InputBox { sf::FloatRect bounds; size_t dom_index; size_t text_index; bool is_button; };

class WindowManager {
private:
    static sf::Color hexToColor(std::string hexStr, sf::Color fallback) {
        if (hexStr.empty()) return fallback;
        if (hexStr[0] == '#') hexStr.erase(0, 1);
        if (hexStr.length() == 3) hexStr = std::string(2, hexStr[0]) + std::string(2, hexStr[1]) + std::string(2, hexStr[2]);
        if (hexStr.length() == 6) {
            try { return sf::Color(std::stoi(hexStr.substr(0, 2), nullptr, 16), std::stoi(hexStr.substr(2, 2), nullptr, 16), std::stoi(hexStr.substr(4, 2), nullptr, 16)); } catch (...) {}
        }
        return fallback;
    }

public:
    static std::string wrapText(const std::string& text, const sf::Font& font, unsigned int characterSize, float maxWidth) {
        std::istringstream words(text); std::string word, wrappedText = "", currentLine = "";
        sf::Text measureText(font, "", characterSize);
        while (words >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            measureText.setString(testLine);
            if (measureText.getLocalBounds().size.x > maxWidth) {
                wrappedText += currentLine + "\n"; currentLine = word;
            } else { currentLine = testLine; }
        }
        return wrappedText + currentLine;
    }

    static std::string open_browser_window(const std::string& current_url, std::vector<DOMNode>& dom) {
        sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "My C++ Browser");
        sf::Font font; 
        if (!font.openFromFile("unicode.ttf")) return "";

        sf::RectangleShape top_bar(sf::Vector2f(800.f, 60.f)); top_bar.setFillColor(sf::Color(200, 200, 200));
        sf::RectangleShape back_btn(sf::Vector2f(40.f, 40.f)); back_btn.setFillColor(sf::Color(170, 170, 170)); back_btn.setOutlineColor(sf::Color(130, 130, 130)); back_btn.setOutlineThickness(2.f); back_btn.setPosition(sf::Vector2f(10.f, 10.f));
        sf::Text back_text(font, "<", 24); back_text.setFillColor(sf::Color::Black); back_text.setPosition(sf::Vector2f(22.f, 15.f));
        sf::RectangleShape address_box(sf::Vector2f(730.f, 40.f)); address_box.setFillColor(sf::Color::White); address_box.setOutlineColor(sf::Color(150, 150, 150)); address_box.setOutlineThickness(2.f); address_box.setPosition(sf::Vector2f(60.f, 10.f));
        
        std::string input_string = current_url;
        sf::Text address_text(font, input_string, 20); address_text.setFillColor(sf::Color::Black); address_text.setPosition(sf::Vector2f(70.f, 15.f));

        std::vector<sf::Text> render_tree_text;
        std::vector<sf::Sprite> render_tree_images;
        std::vector<sf::RectangleShape> render_tree_shapes;
        std::list<sf::Texture> texture_storage; 
        std::vector<Hitbox> link_hitboxes; 
        std::vector<InputBox> form_hitboxes;
        
        float current_y = 90.f; 
        bool is_typing_address = false;
        int focused_input_idx = -1;
        std::string return_action = "";

        for (size_t i = 0; i < dom.size(); i++) {
            auto& node = dom[i];

            if (node.is_image && !node.image_data.empty()) {
                texture_storage.emplace_back();
                if (texture_storage.back().loadFromMemory(node.image_data.data(), node.image_data.size())) {
                    sf::Sprite sprite(texture_storage.back());
                    sprite.setPosition(sf::Vector2f(70.f, current_y)); 
                    if (sprite.getLocalBounds().size.x > 660.f) {
                        float scale = 660.f / sprite.getLocalBounds().size.x;
                        sprite.setScale(sf::Vector2f(scale, scale));
                    }
                    render_tree_images.push_back(sprite);
                    current_y += sprite.getGlobalBounds().size.y + 15.f;
                }
                continue; 
            }

            if (node.is_input) {
                sf::RectangleShape fieldBox;
                if (node.is_button) {
                    fieldBox.setSize(sf::Vector2f(200.f, 35.f));
                    fieldBox.setFillColor(sf::Color(220, 220, 220));
                    fieldBox.setOutlineColor(sf::Color(100, 100, 100));
                } else {
                    fieldBox.setSize(sf::Vector2f(400.f, 35.f));
                    fieldBox.setFillColor(sf::Color::White);
                    fieldBox.setOutlineColor(sf::Color(150, 150, 150));
                }
                fieldBox.setOutlineThickness(1.f);
                fieldBox.setPosition(sf::Vector2f(70.f, current_y));
                
                sf::Text ui_text(font, node.is_button ? node.text : node.input_value, 18);
                ui_text.setFillColor(sf::Color::Black);
                ui_text.setPosition(sf::Vector2f(75.f, current_y + 5.f));

                render_tree_shapes.push_back(fieldBox);
                size_t text_idx = render_tree_text.size();
                render_tree_text.push_back(ui_text);

                form_hitboxes.push_back({fieldBox.getGlobalBounds(), i, text_idx, node.is_button});
                current_y += 50.f;
                continue;
            }

            unsigned int size = 16; sf::Color color = sf::Color::Black;
            if (node.tag == "h1" || node.tag == "h2") { size = 24; color = sf::Color(50, 50, 150); } 
            else if (node.tag == "a") color = sf::Color::Blue; 
            else if (node.tag == "b" || node.tag == "strong") color = sf::Color(150, 0, 0); 

            if (node.css_font_size > 0) size = node.css_font_size;
            if (!node.css_color.empty()) color = hexToColor(node.css_color, color);

            sf::Text ui_text(font, wrapText(node.text, font, size, 660.f), size);
            ui_text.setFillColor(color); ui_text.setPosition(sf::Vector2f(70.f, current_y));
            render_tree_text.push_back(ui_text);

            if (node.tag == "a" && !node.link_url.empty()) link_hitboxes.push_back({ui_text.getGlobalBounds(), node.link_url});
            current_y += ui_text.getLocalBounds().size.y + 15.f; 
        }

        float total_height = std::max(400.f, current_y - 30.f);
        sf::RectangleShape div_box(sf::Vector2f(700.f, total_height));
        div_box.setFillColor(sf::Color(240, 240, 240)); div_box.setPosition(sf::Vector2f(50.f, 70.f));

        sf::View ui_view = window.getDefaultView(); sf::View dom_view = window.getDefaultView(); 
        float scroll_y = dom_view.getCenter().y, min_scroll = scroll_y, max_scroll = std::max(min_scroll, total_height + 150.f - (window.getSize().y / 2.f));

        while (window.isOpen()) {
            while (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) window.close();
                else if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                        scroll_y = std::clamp(scroll_y - scrollEvent->delta * 40.f, min_scroll, max_scroll);
                        dom_view.setCenter(sf::Vector2f(dom_view.getCenter().x, scroll_y));
                    }
                }
                else if (const auto* clickEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (clickEvent->button == sf::Mouse::Button::Left) {
                        sf::Vector2f static_mouse_pos(clickEvent->position.x, clickEvent->position.y);
                        sf::Vector2f world_mouse_pos = window.mapPixelToCoords(sf::Vector2i(clickEvent->position.x, clickEvent->position.y), dom_view);
                        
                        is_typing_address = false; focused_input_idx = -1;
                        address_box.setOutlineColor(sf::Color(150, 150, 150));

                        if (back_btn.getGlobalBounds().contains(static_mouse_pos)) { return_action = "BACK"; window.close(); }
                        else if (address_box.getGlobalBounds().contains(static_mouse_pos)) {
                            is_typing_address = true; address_box.setOutlineColor(sf::Color::Blue);
                        } else {
                            bool hit_form = false;
                            for (const auto& box : form_hitboxes) {
                                if (box.bounds.contains(world_mouse_pos)) {
                                    hit_form = true;
                                    if (box.is_button) {
                                        std::string target_action = dom[box.dom_index].form_action;
                                        std::string query = target_action + "?";
                                        for (const auto& node : dom) {
                                            if (node.is_input && !node.is_button && node.form_action == target_action) {
                                                std::string val = node.input_value;
                                                std::replace(val.begin(), val.end(), ' ', '+'); 
                                                query += node.input_name + "=" + val + "&";
                                            }
                                        }
                                        if (query.back() == '&' || query.back() == '?') query.pop_back();
                                        return_action = "FORM_SUBMIT:" + query;
                                        window.close();
                                    } else {
                                        focused_input_idx = box.dom_index;
                                    }
                                }
                            }
                            if (!hit_form) {
                                for (const auto& box : link_hitboxes) {
                                    if (box.bounds.contains(world_mouse_pos)) { return_action = "LINK:" + box.url; window.close(); }
                                }
                            }
                        }
                    }
                }
                else if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                    if (is_typing_address) {
                        if (textEvent->unicode == '\b' && !input_string.empty()) input_string.pop_back();
                        else if (textEvent->unicode == 13) { return_action = "NEW_DOMAIN:" + input_string; window.close(); }
                        else if (textEvent->unicode >= 32 && textEvent->unicode < 127) input_string += static_cast<char>(textEvent->unicode);
                        address_text.setString(input_string);
                    } else if (focused_input_idx != -1) {
                        auto& node = dom[focused_input_idx];
                        if (textEvent->unicode == '\b' && !node.input_value.empty()) node.input_value.pop_back();
                        else if (textEvent->unicode >= 32 && textEvent->unicode < 127) node.input_value += static_cast<char>(textEvent->unicode);
                        
                        for (auto& box : form_hitboxes) {
                            if (box.dom_index == focused_input_idx) {
                                render_tree_text[box.text_index].setString(node.input_value);
                            }
                        }
                    }
                }
            }
            
            window.clear(sf::Color::White);
            window.setView(dom_view); 
            window.draw(div_box);
            for (const auto& shape : render_tree_shapes) window.draw(shape);
            for (const auto& sprite : render_tree_images) window.draw(sprite); 
            for (const auto& text : render_tree_text) window.draw(text);       
            
            window.setView(ui_view);
            window.draw(top_bar); window.draw(back_btn); window.draw(back_text);
            window.draw(address_box); window.draw(address_text);
            window.display();
        }
        return return_action;
    }
};
#endif