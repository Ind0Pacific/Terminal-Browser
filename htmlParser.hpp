#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <iostream>
#include <string>
#include <vector>

struct DOMNode {
    std::string tag;
    std::string text;
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
        return text;
    }

public:
  static std::vector<DOMNode> parseDOM(const std::string &html) {
    std::vector<DOMNode> dom_tree;
    size_t cursor = 0;
    bool ignore_text = false;
    std::string current_tag = "span"; 

    while (cursor < html.length()) {
      size_t openBracket = html.find('<', cursor);

      if (openBracket > cursor) {
        std::string text = html.substr(cursor, openBracket - cursor);
        text.erase(0, text.find_first_not_of(" \n\r\t")); 
        
        if (!text.empty() && !ignore_text) {
          text = decodeEntities(text);
          dom_tree.push_back({current_tag, text});
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
        } else {
          size_t spacePos = tagContent.find(' ');
          std::string tagName = tagContent.substr(0, spacePos);
          if (tagName == "script" || tagName == "style") ignore_text = true;
          else current_tag = tagName; 
        }
      }
      cursor = closeBracket + 1;
    }
    return dom_tree;
  }
};
#endif