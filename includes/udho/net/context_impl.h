#ifndef UDHO_NET_CONTEXT_IMPL_H
#define UDHO_NET_CONTEXT_IMPL_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <udho/net/fwd.h>

namespace udho{
namespace net{

class context_impl: public std::enable_shared_from_this<context_impl>{
    using handler_type = std::function<void (boost::system::error_code, std::size_t)>;

    udho::net::types::socket            _socket;
    udho::net::types::strand            _strand;
    udho::net::types::headers::response _response;
    handler_type                        _handler;
    boost::asio::streambuf              _streambuf;
    std::ostream                        _stream;

    friend class udho::net::context;

    context_impl() = delete;
    context_impl(const context&) = delete;
    context_impl(udho::net::types::socket&& socket): _socket(std::move(socket)), _strand(_socket.get_executor()), _stream(&_streambuf) { }

    auto shared_from_this(){
        return std::enable_shared_from_this<context_impl>::shared_from_this();
    }

    public:
        template <typename CharT>
        void write(const std::string_view& str){
            std::copy(str.begin(), str.end(), std::ostream_iterator<CharT>(_stream));
            boost::asio::async_write(
                _socket, _streambuf,
                boost::asio::bind_executor(_strand, std::bind(&context_impl::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2))
            );
        }

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred){

        }
};

}
}


#endif // UDHO_NET_CONTEXT_IMPL_H

