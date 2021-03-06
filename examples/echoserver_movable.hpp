#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "boost/asio.hpp"
#include "winext/named_pipe_protocol.hpp"
#include "echoserver_session.hpp"

// Used as example and testing

class server_movable
{
public:
  server_movable(net::io_context& io_context, named_pipe_protocol<net::io_context::executor_type>::endpoint ep)
    : acceptor_(io_context, ep)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    std::cout<<"do_accept"<<std::endl;
    acceptor_.async_accept(
        [this](boost::system::error_code ec, named_pipe_protocol<net::io_context::executor_type>::server_pipe socket)
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

  named_pipe_protocol<net::io_context::executor_type>::acceptor acceptor_;
};