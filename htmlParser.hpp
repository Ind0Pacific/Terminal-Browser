#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <cstddef>
#include <iostream>
#include <string>

class htmlParser {
public:
  static std::string extractText(const std::string &html) {
    std::string page_text = "";
    size_t cursor = 0;
    
    bool ignore_text = false;

    while (cursor < html.length()) {
      size_t openBracket = html.find('<', cursor);


      if (openBracket > cursor) {
        std::string text = html.substr(cursor, openBracket - cursor);
        text.erase(0, text.find_first_not_of(" \n\r\t")); 
        
        if (!text.empty() && !ignore_text) {
          page_text += text + " ";
        }
      }
      
      if (openBracket == std::string::npos) break;
      
      size_t closeBracket = html.find('>', openBracket);
      if (closeBracket == std::string::npos) break;
      
      std::string tagContent = html.substr(openBracket + 1, closeBracket - openBracket - 1);
      
      if (!tagContent.empty()) {
        if (tagContent[0] == '/') {
          std::string tagName = tagContent.substr(1);
          if (tagName == "script" || tagName == "style") {
              ignore_text = false;
          }
        } else {

          size_t spacePos = tagContent.find(' ');
          std::string tagName = tagContent.substr(0, spacePos);
          
          if (tagName == "script" || tagName == "style") {
              ignore_text = true;
          }
        }
      }

      cursor = closeBracket + 1;
    }
    return page_text;
  }
};
#endif