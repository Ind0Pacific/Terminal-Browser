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