#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <thread>

#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include "winext/named_pipe_protocol.hpp"

namespace net = boost::asio;
using namespace winext;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(named_pipe_protocol<net::io_context::executor_type>::server_pipe pipe)
    : pipe_(std::move(pipe))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    pipe_.async_read_some(net::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    net::async_write(pipe_, net::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            do_read();
          }
        });
  }

  named_pipe_protocol<net::io_context::executor_type>::server_pipe pipe_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(net::io_context& io_context, named_pipe_protocol<net::io_context::executor_type>::endpoint ep)
    : acceptor_(io_context, ep),
      pipe_(io_context)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(pipe_,
        [this](std::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(pipe_))->start();
          }

          do_accept();
        });
  }

  named_pipe_protocol<net::io_context::executor_type>::acceptor acceptor_;
  named_pipe_protocol<net::io_context::executor_type>::server_pipe pipe_;
};

void client(int i){
    try
    {
        net::io_context io_context;
        named_pipe_protocol<net::io_context::executor_type>::endpoint ep("\\\\.\\pipe\\mynamedpipe");
        named_pipe_protocol<net::io_context::executor_type>::client_pipe pipe(io_context);
        pipe.connect(ep);

        const int max_length = 1024;

        std::string myMessage = std::string("myMessage") + std::to_string(i) + "hi";
        //char request[max_length];
        //size_t request_length = std::strlen(request);

        net::write(pipe, net::buffer(myMessage.c_str(), myMessage.length()));

        char reply[max_length];
        size_t reply_length = net::read(pipe,
            net::buffer(reply, myMessage.length()));
        std::string replyMsg = std::string(reply, myMessage.length());

        std::cout << "check: " << myMessage << std::endl;
        if(myMessage != replyMsg){
            std::cout << "not match" << std::endl;
        }

    }  catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}


int main(int argc, char* argv[])
{
  try
  {

    net::io_context io_context;

    named_pipe_protocol<net::io_context::executor_type>::endpoint ep("\\\\.\\pipe\\mynamedpipe");

    server s(io_context, ep);

    std::vector<std::thread> server_threads;
    auto count = 1; //std::thread::hardware_concurrency() * 2;

    for(int n = 0; n < count; ++n)
    {
        server_threads.emplace_back([&]
        {
            io_context.run();
        });
    }

    // lauch client threads
    auto client_count = 3;
    std::vector<std::thread> client_threads;
    for(int n = 0; n < client_count; ++n)
    {
        client_threads.emplace_back([n]{
            client(n);
        });
    }

    for(auto& thread : client_threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }

    // finish server
    io_context.stop();
    for(auto& thread : server_threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}