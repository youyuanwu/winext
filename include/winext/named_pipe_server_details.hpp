#ifndef ASIO_NAMED_PIPE_DETAILS_HPP
#define ASIO_NAMED_PIPE_DETAILS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/coroutine.hpp"

#include <iostream>

namespace asio{
namespace details{

// For server pipe.
// create namedpipe and event, and make server listen on this pipe
// and return handles to caller. caller is responsible to free the handles.
inline void accept_detail(
        HANDLE& pipe_ret,
        HANDLE& event_ret,
        OVERLAPPED & overlapped,
        std::string const &endpoint, 
        asio::error_code & ret)
{
    //std::cout << "accept_detail" << std::endl;
    // Create event and assign to pipe and its overlap struct
    HANDLE hEvent = CreateEvent( 
    NULL,    // default security attribute 
    TRUE,    // manual-reset event  ??
    TRUE,    // initial state = signaled 
    NULL);   // unnamed event object

    if(hEvent == NULL)
    {
        DWORD last_error = ::GetLastError();
        asio::error_code pec = asio::error_code(last_error,
                asio::error::get_system_category());
        ret = pec;
        return;
    }

    const int bufsize = 512;

    HANDLE hPipe = CreateNamedPipe( 
        endpoint.c_str(),        // pipe name 
        PIPE_ACCESS_DUPLEX |     // read/write access 
        FILE_FLAG_OVERLAPPED,    // overlapped mode 
        PIPE_TYPE_MESSAGE |      // message-type pipe 
        PIPE_READMODE_MESSAGE |  // message-read mode 
        PIPE_WAIT,               // blocking mode 
        PIPE_UNLIMITED_INSTANCES,               // number of instances 
        bufsize*sizeof(TCHAR),   // output buffer size 
        bufsize*sizeof(TCHAR),   // input buffer size 
        0,            // client time-out 
        NULL);                   // default security attributes 

    if (hPipe == INVALID_HANDLE_VALUE) 
    {
        // printf("CreateNamedPipe failed with %d.\n", GetLastError());
        DWORD last_error = ::GetLastError();
        asio::error_code pec = asio::error_code(last_error,
                asio::error::get_system_category());
        ret = pec;
        return;
    }
    // printf("created namedpipe\n");
    
    // pipe takes ownership of handles
    //pipe->assign(hPipe);

    // wait for client to connect
    overlapped.hEvent = hEvent;
    bool fConnected = false; 
    fConnected = ConnectNamedPipe(hPipe, &overlapped);
    // Overlapped ConnectNamedPipe should return zero. 
    if (fConnected)
    {
        // printf("ConnectNamedPipe failed with %d.\n", GetLastError());
        DWORD last_error = ::GetLastError();
        asio::error_code pec = asio::error_code(last_error,
                asio::error::get_system_category());
        ret = pec;
        return;
    }

    switch (GetLastError()) 
    { 
        // The overlapped connection in progress. 
        case ERROR_IO_PENDING: 
            // OS should have triggered callback
            // std::cout<< "pipe io pending" <<std::endl;
            break; 
        // Client is already connected, so signal an event. 
        case ERROR_PIPE_CONNECTED:
            // std::cout<< "pipe already connected" <<std::endl;
            if (SetEvent(hEvent)) // trigger callback here
                break; 
        // If an error occurs during the connect operation... 
        default: 
        {
            // printf("ConnectNamedPipe failed with %d.\n", GetLastError());
            DWORD last_error = ::GetLastError();
            asio::error_code pec = asio::error_code(last_error,
                    asio::error::get_system_category());
            ret = pec;
            return;
        }
    }
    pipe_ret = hPipe;
    event_ret = hEvent;
    // std::cout << "accept_detail: pipe constructed" << std::endl;
}

// handler of signature void(error_code, pipe)
template<typename Executor>
class async_move_accept_op : ::asio::coroutine {
public:
    typedef std::string endpoint_type;

    async_move_accept_op(server_named_pipe<Executor> *pipe, endpoint_type endpoint):
        pipe_(pipe), endpoint_(endpoint){}

    template <typename Self>
    void operator()(Self& self, asio::error_code ec = {}) {
        // std::cout << "async_move_accept_op" << std::endl;
        if (ec) {
            // std::cout << "async_move_accept_op has error" << std::endl;
            self.complete(ec, std::move(*pipe_));
            return;
        }
        HANDLE hPipe = NULL;
        HANDLE hEvent = NULL;

        // TODO: maybe this does not need coro or compose.
        ASIO_CORO_REENTER(*this) {
            //std::cout << "async_move_accept_op start" << std::endl;
            // initialize the pipe and assign handle to pipe_ owner
            details::accept_detail(hPipe, hEvent, pipe_->oOverlap_,endpoint_,ec);
            if(ec){
                std::cout << "async_move_accept_op accept detail error: "<< ec.message() << std::endl;
                self.complete(ec,std::move(*pipe_));
                return;    
            }
            pipe_->assign(hPipe);
            pipe_->o_.assign(hEvent);

            // std::cout << "async_move_accept_op wait" << std::endl;
            ASIO_CORO_YIELD {
                pipe_->o_.async_wait([self=std::move(self), p = pipe_](asio::error_code ec) mutable { 
                    // std::cout << "async_move_accept_op async_wait handle called" << std::endl;
                    // if(ec){
                    //     std::cout << "async_move_accept_op async_wait handle error: "<< ec.message() << std::endl;
                    // }
                    self.complete(ec, std::move(*p));
                });
            }
            return;
        }
    }

private:
    // pipe for movable case is the pipe holder in acceptor, which needs to moved to handler function,
    // so that to free acceptor pipe holder to handle the next connection.
    server_named_pipe<Executor> *pipe_;
    endpoint_type endpoint_;
};

// handler of signature void(error_code)
template<typename Executor>
class async_accept_op : ::asio::coroutine {
public:
    typedef std::string endpoint_type;

    async_accept_op(server_named_pipe<Executor> *pipe, endpoint_type endpoint):
        pipe_(pipe), endpoint_(endpoint){}

    template <typename Self>
    void operator()(Self& self, asio::error_code ec = {}) {
        // std::cout << "async_move_accept_op" << std::endl;
        if (ec) {
            // std::cout << "async_move_accept_op has error" << std::endl;
            self.complete(ec);
            return;
        }
        HANDLE hPipe = NULL;
        HANDLE hEvent = NULL;

        // TODO: maybe this does not need coro or compose.
        ASIO_CORO_REENTER(*this) {
            //std::cout << "async_move_accept_op start" << std::endl;
            // initialize the pipe and assign handle to pipe_ owner
            details::accept_detail(hPipe, hEvent, pipe_->oOverlap_,endpoint_,ec);
            if(ec){
                std::cout << "async_move_accept_op accept detail error: "<< ec.message() << std::endl;
                self.complete(ec);
                return;    
            }
            pipe_->assign(hPipe);
            pipe_->o_.assign(hEvent);

            // std::cout << "async_move_accept_op wait" << std::endl;
            ASIO_CORO_YIELD {
                pipe_->o_.async_wait([self=std::move(self)](asio::error_code ec) mutable { 
                    // std::cout << "async_move_accept_op async_wait handle called" << std::endl;
                    // if(ec){
                    //     std::cout << "async_move_accept_op async_wait handle error: "<< ec.message() << std::endl;
                    // }
                    self.complete(ec);
                });
            }
            return;
        }
    }

private:
    // pipe for movable case is the pipe holder in acceptor, which needs to moved to handler function,
    // so that to free acceptor pipe holder to handle the next connection.
    server_named_pipe<Executor> *pipe_;
    endpoint_type endpoint_;
};


} // namespace details
} // namespace asio

#endif //ASIO_NAMED_PIPE_DETAILS_HPP