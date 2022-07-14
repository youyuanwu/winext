// Named pipe for server and client

#ifndef ASIO_NAMED_PIPE_HPP
#define ASIO_NAMED_PIPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include "boost/asio/any_io_executor.hpp"
#include "boost/asio/windows/stream_handle.hpp"
#include "boost/asio/windows/object_handle.hpp"

#include "winext/named_pipe_client_details.hpp"

#include <list>
#include <memory>
#include <map>

#include <iostream> //debug

namespace winext {

// maybe just a alias to stream handle

// template <typename Executor = any_io_executor>
// class named_pipe_protocol;

template <typename Executor = boost::asio::any_io_executor>
class named_pipe_acceptor;

// pipe for server
template <typename Executor = boost::asio::any_io_executor>
class server_named_pipe : public boost::asio::windows::basic_stream_handle<Executor> {
public:
    typedef Executor executor_type;

    /// Rebinds the socket type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The socket type when rebound to the specified executor.
        typedef server_named_pipe<Executor1> other;
    };

    server_named_pipe(executor_type& ex) : boost::asio::windows::basic_stream_handle<executor_type>(ex), oOverlap_(),o_(ex){}

    template<typename ExecutionContext>
    server_named_pipe(ExecutionContext& context, typename boost::asio::constraint<
        boost::asio::is_convertible<ExecutionContext&, boost::asio::execution_context&>::value
      >::type = 0) : boost::asio::windows::basic_stream_handle<executor_type>(context.get_executor()), oOverlap_(),o_(context.get_executor()){}

    server_named_pipe(server_named_pipe&& other)
    : boost::asio::windows::basic_stream_handle<executor_type>(std::move(other))
        ,o_(std::move(other.o_)),oOverlap_(other.oOverlap_)
    {
        std::cout<< "pipe moved" << std::endl;
    }

    void assign_event_handle(HANDLE hEvent){
        // not used for now
        // this->o_.assign(hEvent);
        this->oOverlap_.hEvent = hEvent;
    }

    // oOverlap passed into ConnectNamedPipe needs to be preserved before async calls finish,
    // otherwise there is a invalid memory write in the winio service.
    OVERLAPPED oOverlap_; // TODO make private

    // not used in v1
    boost::asio::windows::basic_object_handle<executor_type> o_;
private:
};


template <typename Executor = boost::asio::any_io_executor>
class client_named_pipe : public boost::asio::windows::basic_stream_handle<Executor>{
public:
    typedef Executor executor_type;
    typedef std::string endpoint_type;

    client_named_pipe(executor_type& ex) : boost::asio::windows::basic_stream_handle<executor_type>(ex){}

    template<typename ExecutionContext>
    client_named_pipe(ExecutionContext& context, typename boost::asio::constraint<
        boost::asio::is_convertible<ExecutionContext&, boost::asio::execution_context&>::value
      >::type = 0) : boost::asio::windows::basic_stream_handle<executor_type>(context.get_executor()){}

    BOOST_ASIO_SYNC_OP_VOID connect(const endpoint_type& endpoint,
        boost::system::error_code& ec){
        
        if(boost::asio::windows::basic_stream_handle<executor_type>::is_open()){
           boost::asio::windows::basic_stream_handle<executor_type>::close();
        }

        HANDLE hPipe = NULL;
        details::client_connect(ec, hPipe, endpoint);

        if(ec)
        {
            BOOST_ASIO_SYNC_OP_VOID_RETURN(ec); 
        }
        // assign the pipe to super class
        boost::asio::windows::basic_stream_handle<executor_type>::assign(hPipe);
        BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);    
    }

    void connect(const endpoint_type &endpoint) {
        boost::system::error_code ec;
        this->connect(endpoint,ec);
        boost::asio::detail::throw_error(ec, "connect");
    }
};

} // namespace asio

#endif // ASIO_NAMED_PIPE_HPP