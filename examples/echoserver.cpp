#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include "winext/named_pipe_protocol.hpp"

using namespace asio;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(named_pipe_protocol<io_context::executor_type>::server_pipe pipe)
    : pipe_(std::move(pipe))
  {
    std::cout << "session constructed" << std::endl;
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    std::cout<<"do_read"<<std::endl;
    auto self(shared_from_this());
    pipe_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
        {
          std::cout<<"do_read handler"<<std::endl;
          if (!ec)
          {
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length)
  {
    std::cout<<"do_write"<<std::endl;
    auto self(shared_from_this());
    asio::async_write(pipe_, asio::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          std::cout<<"do_write handler"<<std::endl;
          if (!ec)
          {
            do_read();
          }
        });
  }

  named_pipe_protocol<io_context::executor_type>::server_pipe pipe_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(asio::io_context& io_context, named_pipe_protocol<asio::io_context::executor_type>::endpoint ep)
    : acceptor_(io_context, ep),
      pipe_(io_context)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    std::cout<<"do_accept"<<std::endl;
    acceptor_.async_accept(pipe_,
        [this](std::error_code ec)
        {
          std::cout<<"do_accept handler"<<std::endl;
          if (!ec)
          {
            std::make_shared<session>(std::move(pipe_))->start();
          }

          do_accept();
        });
  }

  named_pipe_protocol<asio::io_context::executor_type>::acceptor acceptor_;
  named_pipe_protocol<io_context::executor_type>::server_pipe pipe_;
};

int main(int argc, char* argv[])
{
  try
  {

    asio::io_context io_context;

    named_pipe_protocol<asio::io_context::executor_type>::endpoint ep("\\\\.\\pipe\\mynamedpipe");

    server s(io_context, ep);

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}