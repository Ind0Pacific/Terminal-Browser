#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <cstddef>
#include <iostream>
#include <string>

class htmlParser
{
public:
    static void tokenizePrint(const std::string &html)
    {
        std::cout << "\n--- Starting HTML Tokenization ---\n";
        size_t cursor = 0;

        while (cursor < html.length())
        {
            size_t openBracket = html.find('<', cursor);

            if (openBracket > cursor)
            {
                std::string text = html.substr(cursor, openBracket - cursor);

                text.erase(0, text.find_first_not_of(" \n\r\t"));
                if (!text.empty())
                {
                    std::cout << "[TEXT NODE] " << text << "\n";
                }
            }
            if (openBracket == std::string::npos)
                break;

            size_t closeBracket = html.find('>', openBracket);
            if (closeBracket == std::string::npos)
                break;

            std::string tagContent =
                html.substr(openBracket + 1, closeBracket - openBracket - 1);

            if (!tagContent.empty() && tagContent[0] == '/')
            {
                std::cout << "[CLOSE TAG] " << tagContent.substr(1) << "\n";
            }
            else
            {
                std::cout << "[OPEN TAG]  " << tagContent << "\n";
            }
            cursor = closeBracket + 1;
        }
        std::cout << "--- Tokenization Complete ---\n\n";
    }
};
#endif