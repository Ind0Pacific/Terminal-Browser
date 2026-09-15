#define ASIO_STANDALONE
#include "htmlParser.hpp"
#include "windowManager.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <fstream> 
#include <sstream>
#include <string>
#include <vector>

void parseUrl(const std::string& url, std::string& host, std::string& path) {
    std::string temp = url;
    if (temp.find("https://") == 0) temp = temp.substr(8);
    else if (temp.find("http://") == 0) temp = temp.substr(7);

    size_t slash_pos = temp.find('/');
    if (slash_pos != std::string::npos) {
        host = temp.substr(0, slash_pos);
        path = temp.substr(slash_pos);
    } else {
        host = temp;
        path = "/";
    }
}

std::string cleanUrl(std::string url) {
    size_t pos = 0;
    while ((pos = url.find("&amp;")) != std::string::npos) url.replace(pos, 5, "&");
    return url;
}

std::string decodeChunked(const std::string& raw_body) {
    std::string decoded;
    size_t pos = 0;
    while (pos < raw_body.size()) {
        size_t crlf = raw_body.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        
        std::string hex_str = raw_body.substr(pos, crlf - pos);
        size_t chunk_size = 0;
        try { chunk_size = std::stoul(hex_str, nullptr, 16); } catch (...) { break; } 
        
        if (chunk_size == 0) break;
        pos = crlf + 2; 
        if (pos + chunk_size > raw_body.size()) break;
        decoded += raw_body.substr(pos, chunk_size);
        pos += chunk_size + 2; 
    }
    return decoded.empty() ? raw_body : decoded;
}

std::vector<char> fetchBinaryImage(std::string img_url, std::string current_host) {
    std::string host = current_host, path = img_url;
    if (img_url.find("http") == 0) {
         parseUrl(img_url, host, path);
    } else if (img_url.find("/") != 0) {
         path = "/" + img_url;
    }

    std::vector<char> binary_data;
    try {
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        auto endPoints = resolver.resolve(host, "443");
        
        asio::ssl::context ctx(asio::ssl::context::tls_client);
        ctx.set_default_verify_paths();
        asio::ssl::stream<asio::ip::tcp::socket> secureSocket(io_context, ctx);
        SSL_set_tlsext_host_name(secureSocket.native_handle(), host.c_str());
        
        asio::connect(secureSocket.lowest_layer(), endPoints);
        secureSocket.handshake(asio::ssl::stream_base::client);

        std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
        asio::write(secureSocket, asio::buffer(request));

        asio::streambuf response;
        asio::read_until(secureSocket, response, "\r\n\r\n");
        
        std::istream response_stream(&response);
        std::string header_line;
        bool is_chunked = false;
        while (std::getline(response_stream, header_line) && header_line != "\r") {
            if (header_line.find("chunked") != std::string::npos) is_chunked = true;
        }

        std::stringstream bodyStream;
        if (response.size() > 0) bodyStream << &response;
        asio::error_code error;
        while (asio::read(secureSocket, response, asio::transfer_at_least(1), error)) {
            bodyStream << &response;
        }
        
        std::string raw_body = bodyStream.str();
        if (is_chunked) raw_body = decodeChunked(raw_body);
        
        binary_data.assign(raw_body.begin(), raw_body.end());
    } catch (...) {} 
    
    return binary_data;
}

int main() {
  using namespace std;

  try {
    asio::io_context io_context;
    std::string input_domain;

    cout << "Enter the domain name eg www.google.com" << endl;
    std::getline(std::cin, input_domain);

    if (input_domain.empty()) return -1;

    std::string current_host;
    std::string current_path;
    parseUrl(input_domain, current_host, current_path); 

    std::vector<std::string> history_stack;

    while (true) {
        std::string htmlBody;

        if (current_host == "browser:" && current_path == "//bookmarks") {
            cout << "\n[*] Generating local bookmarks page..." << endl;
            htmlBody = "<style> a { font-size: 20px; } </style> <h1>Saved Bookmarks</h1><br>";
            std::ifstream infile("bookmarks.txt");
            std::string line;
            while (std::getline(infile, line)) {
                if (!line.empty()) {
                    htmlBody += "<a href=\"https://" + line + "\">" + line + "</a><br><br>";
                }
            }
        } 
        else {
            int max_redirects = 5;
            while (max_redirects-- > 0) {
                cout << "\n[*] Fetching " << current_host << current_path << " ..." << endl;
                
                asio::error_code error;
                asio::ip::tcp::resolver resolver(io_context);
                auto endPoints = resolver.resolve(current_host, "443", error);

                if (error) break; 

                asio::ssl::context ctx(asio::ssl::context::tls_client);
                ctx.set_default_verify_paths();
                asio::ssl::stream<asio::ip::tcp::socket> secureSocket(io_context, ctx);
                SSL_set_tlsext_host_name(secureSocket.native_handle(), current_host.c_str());

                asio::connect(secureSocket.lowest_layer(), endPoints);
                secureSocket.handshake(asio::ssl::stream_base::client);

                std::string request = "GET " + current_path + " HTTP/1.1\r\nHost: " + current_host + "\r\nConnection: close\r\n\r\n";
                asio::write(secureSocket, asio::buffer(request));

                asio::streambuf response;
                asio::read_until(secureSocket, response, "\r\n\r\n");
                std::istream response_stream(&response);
                
                std::string http_version;
                int status_code;
                response_stream >> http_version >> status_code;

                std::string header_line;
                std::getline(response_stream, header_line); 

                std::string redirect_url = "";
                bool is_chunked = false;

                while (std::getline(response_stream, header_line) && header_line != "\r") {
                    if (header_line.find("Location: ") == 0 || header_line.find("location: ") == 0) {
                        redirect_url = header_line.substr(10);
                        redirect_url.erase(redirect_url.find_last_not_of(" \r\n") + 1);
                    }
                    if (header_line.find("chunked") != std::string::npos) {
                        is_chunked = true;
                    }
                }

                if (status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308) {
                    cout << "[!] Redirected to: " << redirect_url << endl;
                    if (redirect_url.find("http") == 0) {
                        parseUrl(redirect_url, current_host, current_path);
                    } else {
                        current_path = redirect_url; 
                    }
                    continue; 
                }

                std::stringstream htmlBodyStream;
                if (response.size() > 0) htmlBodyStream << &response;
                while (asio::read(secureSocket, response, asio::transfer_at_least(1), error)) {
                    htmlBodyStream << &response;
                }

                htmlBody = htmlBodyStream.str();
                if (is_chunked) htmlBody = decodeChunked(htmlBody);
                break; 
            }
        }

        if (htmlBody.empty()) break; 

        std::vector<DOMNode> dom_tree = htmlParser::parseDOM(htmlBody); 
        
        if (!(current_host == "browser:" && current_path == "//bookmarks")) {
            cout << "[*] Fetching binary images..." << endl;
            for (auto& node : dom_tree) {
                if (node.is_image && !node.image_url.empty()) {
                    if (node.image_url.find("data:image") == std::string::npos) { 
                        node.image_data = fetchBinaryImage(node.image_url, current_host);
                    }
                }
            }
        }

        std::string full_url = current_host + current_path;
        std::string action = WindowManager::open_browser_window(full_url, dom_tree);

        if (action.empty()) {
            cout << "[*] Browser closed." << endl;
            break; 
        }

        if (action == "BACK") {
            if (!history_stack.empty()) {
                std::string previous_url = history_stack.back();
                history_stack.pop_back(); 
                parseUrl(previous_url, current_host, current_path);
            }
        } 
        else if (action.find("NEW_DOMAIN:") == 0) {
            history_stack.push_back(full_url); 
            std::string typed_url = action.substr(11);
            parseUrl(typed_url, current_host, current_path);
        } 
        else if (action.find("LINK:") == 0) {
            history_stack.push_back(full_url); 
            std::string clicked_url = action.substr(5);
            clicked_url = cleanUrl(clicked_url); 
            if (clicked_url.find("http") == 0) {
                parseUrl(clicked_url, current_host, current_path);
            } else if (clicked_url.find("/") == 0) {
                current_path = clicked_url;
            } else {
                current_path = "/" + clicked_url;
            }
        }
        else if (action.find("FORM_SUBMIT:") == 0) {
            history_stack.push_back(full_url); 
            std::string form_query = action.substr(12);
            form_query = cleanUrl(form_query); 
            if (form_query.find("http") == 0) {
                parseUrl(form_query, current_host, current_path);
            } else if (form_query.find("/") == 0) {
                current_path = form_query;
            } else {
                current_path = "/" + form_query; 
            }
        }
    }
  } catch (std::exception &e) {
    std::cerr << "Network Error: " << e.what() << "\n";
  }

  return 0;
}