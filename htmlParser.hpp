#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>

struct DOMNode
{
    std::string tag;
    std::string text;
    std::string link_url;
    bool is_image = false;
    std::string image_url = "";
    std::vector<char> image_data;

    std::string css_color = "";
    int css_font_size = 0;

    bool is_input = false;
    std::string input_name = "";
    std::string input_value = "";
    bool is_button = false;
    std::string form_action = "";
};

class htmlParser
{
private:
    static std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }
    static std::string decodeEntities(std::string text)
    {
        size_t pos = 0;
        while ((pos = text.find("&amp;")) != std::string::npos)
            text.replace(pos, 5, "&");
        while ((pos = text.find("&copy;")) != std::string::npos)
            text.replace(pos, 6, "©");
        while ((pos = text.find("&lt;")) != std::string::npos)
            text.replace(pos, 4, "<");
        while ((pos = text.find("&gt;")) != std::string::npos)
            text.replace(pos, 4, ">");
        while ((pos = text.find("&quot;")) != std::string::npos)
            text.replace(pos, 6, "\"");
        while ((pos = text.find("&nbsp;")) != std::string::npos)
            text.replace(pos, 6, " ");
        while ((pos = text.find("&#39;")) != std::string::npos)
            text.replace(pos, 5, "'");

        pos = 0;
        while ((pos = text.find("&#", pos)) != std::string::npos)
        {
            size_t end_pos = text.find(';', pos);
            if (end_pos != std::string::npos)
            {
                try
                {
                    int codepoint = std::stoi(text.substr(pos + 2, end_pos - pos - 2));
                    std::string utf8_char;
                    if (codepoint <= 0x7F)
                        utf8_char += static_cast<char>(codepoint);
                    else if (codepoint <= 0x7FF)
                    {
                        utf8_char += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else if (codepoint <= 0xFFFF)
                    {
                        utf8_char += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                        utf8_char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    text.replace(pos, end_pos - pos + 1, utf8_char);
                }
                catch (...)
                {
                    pos = end_pos + 1;
                }
            }
            else
            {
                break;
            }
        }
        return text;
    }

    static std::string getAttribute(const std::string &tagContent, const std::string &attr)
    {
        size_t pos = tagContent.find(attr + "=\"");
        if (pos != std::string::npos)
        {
            size_t start = pos + attr.length() + 2;
            size_t end = tagContent.find('"', start);
            if (end != std::string::npos)
                return tagContent.substr(start, end - start);
        }
        return "";
    }

public:
    static std::vector<DOMNode> parseDOM(const std::string &html)
    {
        std::vector<DOMNode> dom_tree;
        size_t cursor = 0;
        bool ignore_text = false, in_style_tag = false;
        std::string current_tag = "span", current_url = "", current_form_action = "", raw_css = "";

        while (cursor < html.length())
        {
            size_t openBracket = html.find('<', cursor);
            if (openBracket > cursor)
            {
                std::string text = html.substr(cursor, openBracket - cursor);
                std::string original = text;
                text.erase(0, text.find_first_not_of(" \n\r\t"));

                if (in_style_tag)
                    raw_css += original;
                else if (!text.empty() && !ignore_text)
                {
                    text = decodeEntities(text);
                    dom_tree.push_back({current_tag, text, current_url, false, "", {}, "", 0, false, "", "", false, ""});
                }
            }

            if (openBracket == std::string::npos)
                break;
            size_t closeBracket = html.find('>', openBracket);
            if (closeBracket == std::string::npos)
                break;
            std::string tagContent = html.substr(openBracket + 1, closeBracket - openBracket - 1);

            if (!tagContent.empty())
            {
                if (tagContent[0] == '/')
                {
                    std::string tagName = tagContent.substr(1);
                    if (tagName == "script" || tagName == "style")
                    {
                        ignore_text = false;
                        in_style_tag = false;
                    }
                    if (tagName == "form")
                        current_form_action = "";
                    current_tag = "span";
                    current_url = "";
                }
                else
                {
                    size_t spacePos = tagContent.find(' ');
                    std::string tagName = tagContent.substr(0, spacePos);

                    if (tagName == "script")
                        ignore_text = true;
                    else if (tagName == "style")
                    {
                        ignore_text = true;
                        in_style_tag = true;
                    }
                    else
                    {
                        current_tag = tagName;
                        current_url = "";

                        if (tagName == "a")
                            current_url = getAttribute(tagContent, "href");
                        else if (tagName == "img")
                        {
                            std::string img_url = getAttribute(tagContent, "src");
                            if (!img_url.empty())
                                dom_tree.push_back({"img", "[IMG]", "", true, img_url, {}, "", 0, false, "", "", false, ""});
                        }
                        else if (tagName == "form")
                        {
                            current_form_action = getAttribute(tagContent, "action");
                        }
                        else if (tagName == "input")
                        {
                            std::string type = getAttribute(tagContent, "type");
                            std::string name = getAttribute(tagContent, "name");
                            std::string value = getAttribute(tagContent, "value");
                            bool is_btn = (type == "submit" || type == "button");
                            dom_tree.push_back({"input", is_btn ? (value.empty() ? "Submit" : value) : "", "", false, "", {}, "", 0, true, name, value, is_btn, current_form_action});
                        }
                        else if (tagName == "button")
                        {
                            dom_tree.push_back({"button", "Submit", "", false, "", {}, "", 0, true, "", "", true, current_form_action});
                        }
                    }
                }
            }
            cursor = closeBracket + 1;
        }

        std::map<std::string, std::pair<std::string, int>> styles;
        size_t css_cursor = 0;
        while (css_cursor < raw_css.length())
        {
            size_t open_brace = raw_css.find('{', css_cursor);
            if (open_brace == std::string::npos)
                break;
            std::string selectors = raw_css.substr(css_cursor, open_brace - css_cursor);
            size_t close_brace = raw_css.find('}', open_brace);
            if (close_brace == std::string::npos)
                break;
            std::string properties = raw_css.substr(open_brace + 1, close_brace - open_brace - 1);

            std::string parsed_color = "";
            int parsed_size = 0;
            std::stringstream prop_stream(properties);
            std::string prop;
            while (std::getline(prop_stream, prop, ';'))
            {
                size_t colon = prop.find(':');
                if (colon != std::string::npos)
                {
                    std::string key = trim(prop.substr(0, colon)), val = trim(prop.substr(colon + 1));
                    if (key == "color")
                        parsed_color = val;
                    if (key == "font-size")
                    {
                        try
                        {
                            parsed_size = std::stoi(val);
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
            std::stringstream sel_stream(selectors);
            std::string sel;
            while (std::getline(sel_stream, sel, ','))
            {
                sel = trim(sel);
                if (!parsed_color.empty())
                    styles[sel].first = parsed_color;
                if (parsed_size > 0)
                    styles[sel].second = parsed_size;
            }
            css_cursor = close_brace + 1;
        }
        for (auto &node : dom_tree)
        {
            if (styles.count(node.tag))
            {
                node.css_color = styles[node.tag].first;
                node.css_font_size = styles[node.tag].second;
            }
        }
        return dom_tree;
    }
};
#endif