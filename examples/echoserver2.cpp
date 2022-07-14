#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "winext/named_pipe_protocol.hpp"

//using asio::ip::tcp;

using namespace asio;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(named_pipe_protocol<io_context::executor_type>::server_pipe socket)
    : socket_(std::move(socket))
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
    socket_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
        {
          std::cout<<"do_read handler"<<std::endl;
          if (!ec)
          {
            do_write(length);
          }else{
            std::cout<<"do_read handler error: "<< ec.message() <<std::endl;
          }
        });
  }

  void do_write(std::size_t length)
  {
    std::cout<<"do_write"<<std::endl;
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          std::cout<<"do_write handler"<<std::endl;
          if (!ec)
          {
            do_read();
          }
        });
  }

  named_pipe_protocol<io_context::executor_type>::server_pipe socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(asio::io_context& io_context, named_pipe_protocol<asio::io_context::executor_type>::endpoint ep)
    : acceptor_(io_context, ep)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    std::cout<<"do_accept"<<std::endl;
    acceptor_.async_accept(
        [this](asio::error_code ec, named_pipe_protocol<io_context::executor_type>::server_pipe socket)
        {
          if (!ec)
          {
            std::cout<<"do_accept handler ok. making session"<<std::endl;
            std::make_shared<session>(std::move(socket))->start();
          }else{
            std::cout << "accept handler error: "<<ec.message() << std::endl;
          }

          do_accept();
        });
  }

  named_pipe_protocol<asio::io_context::executor_type>::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    // if (argc != 2)
    // {
    //   std::cerr << "Usage: async_tcp_echo_server <port>\n";
    //   return 1;
    // }

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