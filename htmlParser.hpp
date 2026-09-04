#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <iostream>
#include <string>
#include <vector>

struct DOMNode {
    std::string tag;
    std::string text;
    std::string link_url; 
};

class htmlParser {
private:
    static std::string decodeEntities(std::string text) {
        size_t pos = 0;
        while ((pos = text.find("&amp;")) != std::string::npos) text.replace(pos, 5, "&");
        while ((pos = text.find("&copy;")) != std::string::npos) text.replace(pos, 6, "©");
        while ((pos = text.find("&lt;")) != std::string::npos) text.replace(pos, 4, "<");
        while ((pos = text.find("&gt;")) != std::string::npos) text.replace(pos, 4, ">");
        while ((pos = text.find("&quot;")) != std::string::npos) text.replace(pos, 6, "\"");
        while ((pos = text.find("&nbsp;")) != std::string::npos) text.replace(pos, 6, " ");
        while ((pos = text.find("&#39;")) != std::string::npos) text.replace(pos, 5, "'");
        
        pos = 0;
        while ((pos = text.find("&#", pos)) != std::string::npos) {
            size_t end_pos = text.find(';', pos);
            if (end_pos != std::string::npos) {
                try {
                    int codepoint = std::stoi(text.substr(pos + 2, end_pos - pos - 2));
                    std::string utf8_char;
                    if (codepoint <= 0x7F) {
                        utf8_char += static_cast<char>(codepoint);
                    } else if (codepoint <= 0x7FF) {
                        utf8_char += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0xFFFF) {
                        utf8_char += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                        utf8_char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    text.replace(pos, end_pos - pos + 1, utf8_char);
                } catch (...) { pos = end_pos + 1; }
            } else { break; }
        }
        return text;
    }

public:
  static std::vector<DOMNode> parseDOM(const std::string &html) {
    std::vector<DOMNode> dom_tree;
    size_t cursor = 0;
    bool ignore_text = false;
    std::string current_tag = "span"; 
    std::string current_url = ""; 

    while (cursor < html.length()) {
      size_t openBracket = html.find('<', cursor);

      if (openBracket > cursor) {
        std::string text = html.substr(cursor, openBracket - cursor);
        text.erase(0, text.find_first_not_of(" \n\r\t")); 
        
        if (!text.empty() && !ignore_text) {
          text = decodeEntities(text);
          dom_tree.push_back({current_tag, text, current_url});
        }
      }
      
      if (openBracket == std::string::npos) break;
      
      size_t closeBracket = html.find('>', openBracket);
      if (closeBracket == std::string::npos) break;
      
      std::string tagContent = html.substr(openBracket + 1, closeBracket - openBracket - 1);
      
      if (!tagContent.empty()) {
        if (tagContent[0] == '/') {
          std::string tagName = tagContent.substr(1);
          if (tagName == "script" || tagName == "style") ignore_text = false;
          current_tag = "span"; 
          current_url = ""; 
        } else {
          size_t spacePos = tagContent.find(' ');
          std::string tagName = tagContent.substr(0, spacePos);
          
          if (tagName == "script" || tagName == "style") {
              ignore_text = true;
          } else {
              current_tag = tagName; 
              current_url = ""; 

              if (tagName == "a") {
                  size_t hrefPos = tagContent.find("href=\"");
                  if (hrefPos != std::string::npos) {
                      size_t startQuote = hrefPos + 6; 
                      size_t endQuote = tagContent.find('"', startQuote);
                      if (endQuote != std::string::npos) {
                          current_url = tagContent.substr(startQuote, endQuote - startQuote);
                      }
                  }
              }
          }
        }
      }
      cursor = closeBracket + 1;
    }
    return dom_tree;
  }
};
#endif