
#ifndef ASIO_NAMED_PIPE_PROTOCOL_HPP
#define ASIO_NAMED_PIPE_PROTOCOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "winext/named_pipe.hpp"
#include "winext/named_pipe_acceptor.hpp"


namespace winext{

template <typename Executor = any_io_executor>
class named_pipe_protocol{
public:
    typedef named_pipe_acceptor<Executor> acceptor;

    typedef server_named_pipe<Executor> server_pipe;

    typedef client_named_pipe<Executor> client_pipe;

    typedef std::string endpoint;
};

} // namespace asio

#endif // ASIO_NAMED_PIPE_PROTOCOL_HPP