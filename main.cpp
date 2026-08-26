#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
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

    // DNS resolving
    asio::ip::tcp::resolver resolver(io_context);
    auto endPoints = resolver.resolve(input_domain, "443" , error);

    if (error) {
      cout << "DNS Resolution failed: " << error.message() << endl;
      return -1;
    }

    //SSL context
    asio::ssl::context ctx(asio::ssl::context::tls_client);
    ctx.set_default_verify_paths();

    asio::ssl::stream<asio::ip::tcp::socket> secureSocket(io_context , ctx);
    SSL_set_tlsext_host_name(secureSocket.native_handle(), input_domain.c_str()); //SNI

    asio::connect(secureSocket.lowest_layer(), endPoints);
    secureSocket.handshake(asio::ssl::stream_base::client);

    std::string request = "GET / HTTP/1.1\r\n"
                          "Host: " +
                          input_domain +
                          "\r\n"
                          "Accept: text/html\r\n"
                          "Connection: close\r\n\r\n";

    asio::write(secureSocket, asio::buffer(request) ); 

    while (asio::read(secureSocket, response, asio::transfer_at_least(1), error)) {
      cout << &response;
    }

    if (error != asio::error::eof && error != asio::ssl::error::stream_truncated) {
      throw asio::system_error(error);
    }

  } catch (std::exception &e) {
    std::cerr << "Network Error: " << e.what() << "\n";
  }

  return 0;
}