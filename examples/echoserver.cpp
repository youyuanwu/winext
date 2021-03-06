#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "boost/asio.hpp"
#include "winext/named_pipe_protocol.hpp"
#include "echoserver.hpp"

namespace net = boost::asio;
using namespace winext;

int main(int argc, char* argv[])
{
  try
  {

    net::io_context io_context;

    named_pipe_protocol<net::io_context::executor_type>::endpoint ep("\\\\.\\pipe\\mynamedpipe");

    server s(io_context, ep);

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}