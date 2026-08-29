#define ASIO_STANDALONE
#include "htmlParser.hpp"
#include "windowManager.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <sstream>
#include <string>

int main() {
  using namespace std;

  try {
    asio::io_context io_context;
    asio::error_code error;
    asio::streambuf response;
    std::string input_domain;

    cout << "Enter the domain name eg www.google.com" << endl;
    std::getline(std::cin, input_domain);

    if (input_domain.empty()) {
      cout << "Error: Enter the domain" << endl;
      return -1;
    }

    asio::ip::tcp::resolver resolver(io_context);
    auto endPoints = resolver.resolve(input_domain, "443", error);

    if (error) {
      cout << "DNS Resolution failed: " << error.message() << endl;
      return -1;
    }

    asio::ssl::context ctx(asio::ssl::context::tls_client);
    ctx.set_default_verify_paths();

    asio::ssl::stream<asio::ip::tcp::socket> secureSocket(io_context, ctx);
    SSL_set_tlsext_host_name(secureSocket.native_handle(), input_domain.c_str());

    asio::connect(secureSocket.lowest_layer(), endPoints);
    secureSocket.handshake(asio::ssl::stream_base::client);

    std::string request = "GET / HTTP/1.1\r\n"
                          "Host: " +
                          input_domain +
                          "\r\n"
                          "Accept: text/html\r\n"
                          "Connection: close\r\n\r\n";

    asio::write(secureSocket, asio::buffer(request));

    // Strip HTTP Headers
    asio::read_until(secureSocket, response, "\r\n\r\n");
    std::istream response_stream(&response);
    std::string header_line;
    while (std::getline(response_stream, header_line) && header_line != "\r") {
    }

    // Read HTML
    std::stringstream htmlBodyStream;
    if (response.size() > 0) {
      htmlBodyStream << &response;
    }

    while (asio::read(secureSocket, response, asio::transfer_at_least(1), error)) {
      htmlBodyStream << &response;
    }

    if (error != asio::error::eof && error != asio::ssl::error::stream_truncated) {
      throw asio::system_error(error);
    }

    //downloaded HTML
    std::string htmlBody = htmlBodyStream.str();
    std::string website_text = htmlParser::extractText(htmlBody); 

    //  Open GUI
    WindowManager::open_browser_window(input_domain, website_text);

  } catch (std::exception &e) {
    std::cerr << "Network Error: " << e.what() << "\n";
  }

  return 0;
}