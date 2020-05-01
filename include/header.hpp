// Copyright 2019 dimakirol <your_email>

#ifndef INCLUDE_HEADER_HPP_
#define INCLUDE_HEADER_HPP_

#include "root_certificates.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
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

#include <gumbo.h>

#define HTTP_PORT "80"
#define HTTPS_PORT "443"
#define WWW "www."
#define NOT_FOUND "404"
#define MOVED_PERMANENTLY "301 Moved Permanently"
#define SRC "src"
#define HREF "href"
#define CONTENTS "contents"
#define ITEMTYPE "itemtype"
#define TYPE "type"
#define IMAGE "image"
#define HTTPS "https://"
#define HTTP "http://"
#define JPG ".jpg"
#define PNG ".png"
#define GIF ".gif"
#define ICO ".ico"
#define SVG ".svg"
#define SAD_SMILE "://"
#define OCTOTORP "#"

static const uint32_t masha_sleeps_seconds = time(NULL) % 5 + 1;
static const uint32_t odin = 1;
static const uint32_t tri = 3;
static const uint32_t dima_sleeps_seconds = time(NULL) % 9 + 3;
static const uint32_t chetire = 4;
static const uint32_t odin_adzat = 11;
static const uint32_t kirill_sleeps_seconds = time(NULL) % 7 + 1;

namespace po = boost::program_options;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

std::string get_https_page(std::string host, std::string port,
                           std::string target)
{
    std::string web_page("");
    int version = odin_adzat;

    try {
        boost::asio::io_context ioc;
        ssl::context ctx{ssl::context::sslv23_client};
        load_root_certificates(ctx);
        tcp::resolver resolver{ioc};
        ssl::stream<tcp::socket> stream{ioc, ctx};
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                      boost::asio::error::get_ssl_category()};
            throw boost::system::system_error{ec};
        }
        auto const results = resolver.resolve(host, port);
        boost::asio::connect(stream.next_layer(), results.begin(),
                             results.end());
        stream.handshake(ssl::stream_base::client);
        http::request<http::string_body> req{http::verb::get, target,
                                                      version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::write(stream, req);
        boost::beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);
        web_page = boost::beast::buffers_to_string(res.body().data());
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << " in https downloading page "
                               << host << target << std::endl;
        return NOT_FOUND;
    } catch (...) {
        return NOT_FOUND;
    }
    return web_page;
}
std::string get_http_page(std::string host, std::string port,
                          std::string target){
    std::string web_site("");
    try
    {
        int version = odin_adzat;

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
        http::request<http::string_body> req{http::verb::get, target,
                                                     version};
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
        web_site = boost::beast::buffers_to_string(res.body().data());

        // Gracefully close the socket
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.

        if (ec && ec != boost::system::errc::not_connected) {
            throw boost::system::system_error{ec};
        }

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << " in http downloading page "
                               << host << target << std::endl;
        web_site = std::string(NOT_FOUND);
    }
    return  web_site;
}


#endif // INCLUDE_HEADER_HPP_
