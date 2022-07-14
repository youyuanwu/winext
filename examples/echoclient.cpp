#include <cstdlib>
#include <cstring>
#include <iostream>
#include "asio.hpp"
#include "winext/named_pipe_protocol.hpp"

using namespace asio;

enum { max_length = 1024 };

int main(int argc, char* argv[])
{
  try
  {
    asio::io_context io_context;

    named_pipe_protocol<asio::io_context::executor_type>::endpoint ep("\\\\.\\pipe\\mynamedpipe");
    named_pipe_protocol<asio::io_context::executor_type>::client_pipe pipe(io_context);
    pipe.connect(ep);

    std::cout << "Enter message: ";
    char request[max_length];
    std::cin.getline(request, max_length);
    size_t request_length = std::strlen(request);
    asio::write(pipe, asio::buffer(request, request_length));

    char reply[max_length];
    size_t reply_length = asio::read(pipe,
        asio::buffer(reply, request_length));
    std::cout << "Reply is: ";
    std::cout.write(reply, reply_length);
    std::cout << "\n";
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}