#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <string>

int main()
{
    using namespace std;
    try
    {
        asio::io_context io_context;

        std::string input_domain;
        cout << "Enter the domain name eg www.google.com" << endl;
        std::getline(std::cin, input_domain);

        if (input_domain.empty())
        {
            cout << "Error: Enter the domain" << endl;
            return -1;
        }

        asio::ip::tcp::resolver resolver(io_context);
        asio::ip::tcp::resolver::results_type endPoints =
            resolver.resolve(input_domain, "80");

        asio::ip::tcp::socket Socket(io_context);
        asio::connect(Socket, endPoints);

        std::string request = "GET / HTTP/1.1\r\n"
                              "Host: " +
                              input_domain +
                              "\r\n"
                              "Accept: text/html\r\n"
                              "Connection: close\r\n\r\n";

        asio::write(Socket, asio::buffer(request));

        asio::streambuf response;
        asio::error_code error;

        while (asio::read(Socket, response, asio::transfer_at_least(1), error))
        {
            cout << &response;
        }

        if (error != asio::error::eof)
        {
            throw asio::system_error(error);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Network Error: " << e.what() << "\n";
    }

    return 0;
}
