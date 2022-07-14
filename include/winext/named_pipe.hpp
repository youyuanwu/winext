// Named pipe for server and client

#ifndef ASIO_NAMED_PIPE_HPP
#define ASIO_NAMED_PIPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio/detail/config.hpp>
#include <asio/detail/type_traits.hpp>
#include "asio/any_io_executor.hpp"
#include "asio/windows/stream_handle.hpp"
#include "asio/windows/object_handle.hpp"

#include "winext/named_pipe_client_details.hpp"

#include <list>
#include <memory>
#include <map>

#include <iostream> //debug

namespace asio {

// maybe just a alias to stream handle

// template <typename Executor = any_io_executor>
// class named_pipe_protocol;

template <typename Executor = any_io_executor>
class named_pipe_acceptor;

// pipe for server
template <typename Executor = any_io_executor>
class server_named_pipe : public windows::basic_stream_handle<Executor> {
public:
    typedef Executor executor_type;

    /// Rebinds the socket type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The socket type when rebound to the specified executor.
        typedef server_named_pipe<Executor1> other;
    };

    server_named_pipe(executor_type& ex) : windows::basic_stream_handle<executor_type>(ex), oOverlap_(),o_(ex){}

    template<typename ExecutionContext>
    server_named_pipe(ExecutionContext& context, typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value
      >::type = 0) : windows::basic_stream_handle<executor_type>(context.get_executor()), oOverlap_(),o_(context.get_executor()){}

    server_named_pipe(server_named_pipe&& other)
    : windows::basic_stream_handle<executor_type>(std::move(other))
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
    windows::basic_object_handle<executor_type> o_;
private:
};


template <typename Executor = any_io_executor>
class client_named_pipe : public windows::basic_stream_handle<Executor>{
public:
    typedef Executor executor_type;
    typedef std::string endpoint_type;

    client_named_pipe(executor_type& ex) : windows::basic_stream_handle<executor_type>(ex){}

    template<typename ExecutionContext>
    client_named_pipe(ExecutionContext& context, typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value
      >::type = 0) : windows::basic_stream_handle<executor_type>(context.get_executor()){}

    ASIO_SYNC_OP_VOID connect(const endpoint_type& endpoint,
        asio::error_code& ec){
        
        if(windows::basic_stream_handle<executor_type>::is_open()){
            windows::basic_stream_handle<executor_type>::close();
        }

        HANDLE hPipe = NULL;
        details::client_connect(ec, hPipe, endpoint);

        if(ec)
        {
            ASIO_SYNC_OP_VOID_RETURN(ec); 
        }
        // assign the pipe to super class
        windows::basic_stream_handle<executor_type>::assign(hPipe);
        ASIO_SYNC_OP_VOID_RETURN(ec);    
    }

    void connect(const endpoint_type &endpoint) {
        asio::error_code ec;
        this->connect(endpoint,ec);
        asio::detail::throw_error(ec, "connect");
    }
};

} // namespace asio

#endif // ASIO_NAMED_PIPE_HPP