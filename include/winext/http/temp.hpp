#ifndef UNICODE
#define UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <http.h>

#include "winext/http/http.hpp"

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>

#pragma comment(lib, "httpapi.lib")

namespace net = boost::asio;

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(winext::http::basic_http_handle<net::io_context::executor_type> & queue_handle)
        :   queue_handle_(queue_handle),
            buffer_(sizeof(HTTP_REQUEST) + 2048, 0),
            resp_(),
            data_chunk_(),
            reason_(),
            content_type_(),
            body_()
    {
        PHTTP_REQUEST req = winext::http::phttp_request(net::buffer(buffer_));
        HTTP_SET_NULL_ID( &req->RequestId );
    }

    void
    start()
    {
        receive_request();
        // check_deadline();
    }

private:
    std::vector<CHAR>  buffer_; // buffer that backs request
    
    // response block
    HTTP_RESPONSE resp_;
    HTTP_DATA_CHUNK data_chunk_;
    std::string reason_;
    std::string content_type_;
    std::string body_;

    winext::http::basic_http_handle<net::io_context::executor_type> & queue_handle_;

    void
    receive_request()
    {
        std::cout << "connection receive_request" <<std::endl;
        auto self = shared_from_this();
        queue_handle_.async_recieve_request(net::buffer(buffer_),
        [self](boost::system::error_code ec, std::size_t){
            if(ec){
                std::cout << "async_recieve_request failed: " << ec.message() <<std::endl;
            }else{
                self->on_receive();
                // start another connection
                std::make_shared<http_connection>(self->queue_handle_)->start();
            }
        });
    }

    void prepare_resp(USHORT        StatusCode){
        resp_.StatusCode = StatusCode;
        resp_.pReason = reason_.data();
        resp_.ReasonLength = static_cast<USHORT>(reason_.size());

        //
        // Add a known header.
        //
        resp_.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = content_type_.c_str();
        resp_.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = static_cast<USHORT>(content_type_.size());            
    
        if(body_.c_str())
        {
            // 
            // Add an entity chunk.
            //
            data_chunk_.DataChunkType           = HttpDataChunkFromMemory;
            data_chunk_.FromMemory.pBuffer      = (PVOID) body_.c_str();
            data_chunk_.FromMemory.BufferLength = (ULONG) body_.length();

            resp_.EntityChunkCount         = 1;
            resp_.pEntityChunks            = &data_chunk_;
        }
    }

    void on_receive(){
        PHTTP_REQUEST req = winext::http::phttp_request(net::buffer(buffer_));
        // ::on_receive(req, this->queue_handle_.native_handle());
        wprintf(L"Got a request for %ws \n", req->CookedUrl.pFullUrl);
        this->reason_ = "OK";
        this->content_type_ = "text/html";
        this->body_ = "Hey! You hit the server \r\n";
        this->prepare_resp(200);

        auto self = shared_from_this();
        queue_handle_.async_send_response(
            &this->resp_,
            req->RequestId,
            HTTP_SEND_RESPONSE_FLAG_DISCONNECT, // single resp flag
            [self](boost::system::error_code ec, std::size_t){
                std::cout << "async_send_response handler: " << ec.message() << std::endl; 
            });
        
        // boost::system::error_code ec;
        // queue_handle_.send_response(
        //     &this->resp_,
        //     req->RequestId,
        //     HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
        //     ec
        // );
        // if(ec.failed()){
        //     std::cout << "sync send failed:" << ec.message() << std::endl;
        // }
    }
};