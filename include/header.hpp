// Copyright 2019 dimakirol <your_email>

#ifndef INCLUDE_HEADER_HPP_
#define INCLUDE_HEADER_HPP_

#include "root_certificates.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctpl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>
#include <queue>

#include <boost/algorithm/string.hpp>
#include <boost/coroutine/attributes.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gumbo.h>

#define HTTP_PORT "80"
#define HTTPS_PORT "443"


namespace po=boost::program_options;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Performs an HTTP GET and prints the response
void do_session(
        std::string const& host,
        std::string const& port,
        std::string const& target,
        int version,
        net::io_context& ioc,
        ssl::context& ctx,
        net::yield_context yield,
        std::string &web_page)
{
    beast::error_code ec;

    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
    {
        ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        std::cerr << ec.message() << "\n";
        return;
    }

    // Look up the domain name
    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if(ec) {
        std::cerr << "connect" << ": " << ec.message() << "\n";
        web_page = std::string("fail resolve");
        return;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    get_lowest_layer(stream).async_connect(results, yield[ec]);
    if(ec) {
        std::cerr << "connect" << ": " << ec.message() << "\n";
        web_page = std::string("fail connect");
        return;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    stream.async_handshake(ssl::stream_base::client, yield[ec]);
    if(ec) {
        std::cerr << "handshake" << ": " << ec.message() << "\n";
        web_page = std::string("fail handshake");
        return;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(stream, req, yield[ec]);
    if(ec) {
        std::cerr << "write" << ": " << ec.message() << "\n";
        web_page = std::string("fail write");
        return;
    }

    // This buffer is used for reading and must be persisted
    beast::flat_buffer b;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::async_read(stream, b, res, yield[ec]);
    if(ec) {
        std::cerr << "read" << ": " << ec.message() << "\n";
        web_page = std::string("fail read");
        return;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    stream.async_shutdown(yield[ec]);
    if(ec == net::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if(ec) {
        std::cerr << "shutdown" << ": " << ec.message() << "\n";
        web_page = std::string("fail shutdown");
        return;
    }

    std::cout << res << std::endl;
//    web_page = res;
    return;
}

//------------------------------------------------------------------------------

//std::string get_https_page(std::string _host, std::string _port, std::string _target)
//{
//     Check command line arguments.
//    if(argc != 4 && argc != 5)
//    {
//        std::cerr <<
//                  "Usage: http-client-coro-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
//                  "Example:\n" <<
//                  "    http-client-coro-ssl www.example.com 443 /\n" <<
//                  "    http-client-coro-ssl www.example.com 443 / 1.0\n";
//        return EXIT_FAILURE;
//    }
//    auto const host = "www.google.com";//ex: porhub.com
//    auto const port = "443"; // 80! or 443
//    auto const target = "/";
//    int version = 10;
//
//     The io_context is required for all I/O
//    net::io_context ioc;
//
    // The SSL context is required, and holds certificates
//    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
//    load_root_certificates(ctx);
//
    // Verify the remote server's certificate
//    ctx.set_verify_mode(ssl::verify_peer);
//
//    std::string web_page("");
    // Launch the asynchronous operation
//    boost::asio::spawn(ioc, std::bind(
//            &do_session,
//            std::string(host),
//            std::string(port),
//            std::string(target),
//            version,
//            std::ref(ioc),
//            std::ref(ctx),
//            std::placeholders::_1,
//            web_page));
//
    // Run the I/O service. The call will return when
    // the get operation is complete.
//    ioc.run();

    //std::cout << web_page << std::endl;
//    return web_page;
//}
std::string get_http_page(std::string host, std::string port, std::string target){
    try
    {
        // Check command line arguments.
//            if(argc != 4 && argc != 5)
//            {
//                std::cerr <<
//                          "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
//                          "Example:\n" <<
//                          "    http-client-sync www.example.com 80 /\n" <<
//                          "    http-client-sync www.example.com 80 / 1.0\n";
//                return EXIT_FAILURE;
//            }
//        auto const host = "www.google.com";//ex: porhub.com
//        auto const port = "80"; // 80! or 443
//        auto const target = "/"; // path in site (ex video is pornhub.com/kamaz => target = "/kamaz")
        int version = 11;

        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        tcp::socket socket{ioc};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(socket, results.begin(), results.end());

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(socket, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(socket, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the socket
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ec};

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return std::string("");
    }
}
std::string get_https_page(std::string host, std::string port, std::string target){
    try
    {
        // Check command line arguments.
//        if(argc != 4 && argc != 5)
//        {
//            std::cerr <<
//                      "Usage: http-client-sync-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
//                      "Example:\n" <<
//                      "    http-client-sync-ssl www.example.com 443 /\n" <<
//                      "    http-client-sync-ssl www.example.com 443 / 1.0\n";
//            return EXIT_FAILURE;
//        }
//        auto const host = argv[1];
//        auto const port = argv[2];
//        auto const target = argv[3];
        int version = 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        ctx.set_verify_mode(ssl::verify_peer);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
    return "";
}

#endif // INCLUDE_HEADER_HPP_
