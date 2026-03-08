#ifndef UDHO_NET_H11_DETAIL_H
#define UDHO_NET_H11_DETAIL_H

#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

namespace detail{

template <typename StreamT>
struct stream_termination{
    static boost::system::error_code apply(StreamT& stream) {
        boost::system::error_code error;
        stream.cancel(error);
        return error;
    }
};

template <>
struct stream_termination<boost::beast::test::stream>{
    static boost::system::error_code apply(boost::beast::test::stream& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }
};

template <typename StreamT>
struct stream_available{
    static std::size_t apply(StreamT& stream) {
        return stream.available();
    }
};

template <>
struct stream_available<boost::beast::test::stream>{
    static std::size_t apply(boost::beast::test::stream& stream) {
        return 256;
    }
};

template <typename Buffer>
struct transfer_leftover{
    using target_buffer_type = Buffer;

    transfer_leftover(target_buffer_type& target): _target(target), _bytes_transferred(0) {}

    target_buffer_type& operator()(boost::beast::flat_buffer& source, std::size_t content_length) {
        auto src   = source.data();
        auto limit = std::min(content_length, source.size());
        _target.commit(boost::asio::buffer_copy(_target.prepare(limit), src));
        source.consume(limit);
        _bytes_transferred = limit;
        return _target;
    }

    template <typename SourceBufferT>
    target_buffer_type& operator()(SourceBufferT& source) {
        auto src   = source.data();
        auto limit = source.size();
        _target.commit(boost::asio::buffer_copy(_target.prepare(limit), src));
        source.consume(limit);
        _bytes_transferred = limit;
        return _target;
    }

    std::size_t bytes_transferred() const { return _bytes_transferred; }
private:
    target_buffer_type& _target;
    std::size_t         _bytes_transferred;
};

}

}
}
}

#endif // UDHO_NET_H11_DETAIL_H
