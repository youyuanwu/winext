#pragma once

#include "http.h"

namespace winext
{
    namespace http
    {

        // convert asio mutable buffer to request
        template <typename MutableBufferSequence>
        PHTTP_REQUEST phttp_request(const MutableBufferSequence &buffer)
        {
            return (PHTTP_REQUEST)buffer.data();
        }

    }
}