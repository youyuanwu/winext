#include "gtest/gtest.h"
#include "winext/http/temp.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstdio>
#include <utility>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

void invokeAPI(){
    // The io_context is required for all I/O
    net::io_context ioc;

    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    // Look up the domain name
    auto const results = resolver.resolve("localhost", "12356");

    // Make the connection on the IP address we get from a lookup
    stream.connect(results);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, "/winhttpapitest", 11};
    req.set(http::field::host, "localhost");
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

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if(ec && ec != beast::errc::not_connected)
    {
        ASSERT_FALSE(ec.failed());
    }

    http::status result = res.result();
    ASSERT_EQ(http::status::ok, result);
}

TEST(HTTPServer, server) 
{    
  //test_server<server>();
   // init http module
    winext::http::http_initializer init;
    std::wstring url = L"http://localhost:12356/winhttpapitest/";
    
    boost::system::error_code ec;
    net::io_context io_context;
    
    // open queue handle
    winext::http::basic_http_handle<net::io_context::executor_type> queue(io_context);
    queue.assign(winext::http::open_raw_http_queue());
    winext::http::http_simple_url simple_url(queue, url);

    //myserver s(queue);
    std::make_shared<http_connection>(queue)->start();

    int server_count = 1;
    std::vector<std::thread> server_threads;

    for(int n = 0; n < server_count; ++n)
    {
        server_threads.emplace_back([&]
        {
            io_context.run();
        });
    }

    // let server warm up
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // make client call
    invokeAPI();

    // let server listen again
    std::this_thread::sleep_for(std::chrono::seconds(1));

    io_context.stop();
    for(auto& thread : server_threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}