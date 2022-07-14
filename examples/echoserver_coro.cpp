#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <cstdio>
#include "winext/named_pipe_protocol.hpp"

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
namespace this_coro = asio::this_coro;

using namespace asio;

#if defined(ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

awaitable<void> echo(named_pipe_protocol<asio::any_io_executor>::server_pipe socket)
{
  std::cout << "echo invoked" << std::endl;
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(asio::buffer(data), use_awaitable);
      co_await async_write(socket, asio::buffer(data, n), use_awaitable);
    }
  }
  catch (std::exception& e)
  {
    std::printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  // tcp::acceptor acceptor(executor, {tcp::v4(), 55555});
  named_pipe_protocol<asio::any_io_executor>::endpoint ep("\\\\.\\pipe\\mynamedpipe");
  named_pipe_protocol<asio::any_io_executor>::acceptor acceptor(executor, ep);
  for (;;)
  {
    named_pipe_protocol<asio::any_io_executor>::server_pipe socket = co_await acceptor.async_accept(use_awaitable);
     std::cout << "listener spawing" << std::endl;
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int main()
{
  try
  {
    asio::io_context io_context(1);

    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::printf("Exception: %s\n", e.what());
  }
}