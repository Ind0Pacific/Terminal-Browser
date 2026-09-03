#define ASIO_STANDALONE
#include "htmlParser.hpp"
#include "windowManager.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
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

int main() {
  using namespace std;

  try {
    asio::io_context io_context;
    std::string input_domain;

    cout << "Enter the domain name eg www.google.com" << endl;
    std::getline(std::cin, input_domain);

    if (input_domain.empty()) {
      cout << "Error: Enter the domain" << endl;
      return -1;
    }

    std::string current_host = input_domain;
    std::string current_path = "/";

    while (true) {
        int max_redirects = 5;
        std::string htmlBody;

        while (max_redirects-- > 0) {
            cout << "\n[*] Fetching " << current_host << current_path << " ..." << endl;
            
            asio::error_code error;
            asio::ip::tcp::resolver resolver(io_context);
            auto endPoints = resolver.resolve(current_host, "443", error);

            if (error) {
              cout << "DNS Resolution failed: " << error.message() << endl;
              break; 
            }

            asio::ssl::context ctx(asio::ssl::context::tls_client);
            ctx.set_default_verify_paths();

            asio::ssl::stream<asio::ip::tcp::socket> secureSocket(io_context, ctx);
            SSL_set_tlsext_host_name(secureSocket.native_handle(), current_host.c_str());

            asio::connect(secureSocket.lowest_layer(), endPoints);
            secureSocket.handshake(asio::ssl::stream_base::client);

            std::string request = "GET " + current_path + " HTTP/1.1\r\n"
                                  "Host: " + current_host + "\r\n"
                                  "Accept: text/html\r\n"
                                  "Connection: close\r\n\r\n";

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
            while (std::getline(response_stream, header_line) && header_line != "\r") {
                if (header_line.find("Location: ") == 0 || header_line.find("location: ") == 0) {
                    redirect_url = header_line.substr(10);
                    redirect_url.erase(redirect_url.find_last_not_of(" \r\n") + 1);
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

            if (error != asio::error::eof && error != asio::ssl::error::stream_truncated) {
                throw asio::system_error(error);
            }

            htmlBody = htmlBodyStream.str();
            break; 
        }

        if (htmlBody.empty()) break; 

        std::vector<DOMNode> dom_tree = htmlParser::parseDOM(htmlBody); 
        std::string clicked_url = WindowManager::open_browser_window(current_host, dom_tree);

        if (clicked_url.empty()) {
            cout << "[*] Browser closed." << endl;
            break; 
        }

        clicked_url = cleanUrl(clicked_url); 
        if (clicked_url.find("http") == 0) {
            parseUrl(clicked_url, current_host, current_path);
        } else if (clicked_url.find("/") == 0) {
            current_path = clicked_url;
        } else {
            current_path = "/" + clicked_url;
        }
    } 

  } catch (std::exception &e) {
    std::cerr << "Network Error: " << e.what() << "\n";
  }

  return 0;
}