#ifndef ASIO_NAMED_PIPE_CLIENT_DETAILS_HPP
#define ASIO_NAMED_PIPE_CLIENT_DETAILS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace asio{

namespace details{

// client connect to the namedpipe, 
// return the ok handle. Caller is responsible for freeing the handle.
inline void client_connect(asio::error_code & ec, HANDLE& pipe_ret, std::string const & endpoint){
    HANDLE hPipe;
    BOOL fSuccess = FALSE;
    DWORD dwMode;
    DWORD last_error;
    // connect to pipe
    while (1)
    {
        hPipe = CreateFile(
            endpoint.c_str(),  // pipe name
            GENERIC_READ | // read and write access
                GENERIC_WRITE,
            0,                    // no sharing
            NULL,                 // default security attributes
            OPEN_EXISTING,        // opens existing pipe
            FILE_FLAG_OVERLAPPED, // default attributes
            NULL);                // no template file

        // Break if the pipe handle is valid.

        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        // Exit if an error other than ERROR_PIPE_BUSY occurs.

        last_error = ::GetLastError();
        if (last_error != ERROR_PIPE_BUSY)
        {
            // printf("Could not open pipe. GLE=%d\n", GetLastError());
            ec = asio::error_code(last_error,
                    asio::error::get_system_category());
            return;
        }

        // All pipe instances are busy, so wait for 20 seconds.

        if (!WaitNamedPipe(endpoint.c_str(), 20000))
        {
            printf("Could not open pipe: 20 second wait timed out.");
            //return -1;
            ec = asio::error::timed_out;
            return;
        }
    }

    // The pipe connected; change to message-read mode.
    dwMode = PIPE_READMODE_MESSAGE;
    fSuccess = SetNamedPipeHandleState(
        hPipe,   // pipe handle
        &dwMode, // new pipe mode
        NULL,    // don't set maximum bytes
        NULL);   // don't set maximum time
    if (!fSuccess)
    {
        // printf("SetNamedPipeHandleState failed. GLE=%d\n", GetLastError());
        ec = asio::error_code(last_error,
                    asio::error::get_system_category());
        return;
    }
    pipe_ret = hPipe;
}
}// namespace details
}// namespace asio
#endif // ASIO_NAMED_PIPE_CLIENT_DETAILS_HPP