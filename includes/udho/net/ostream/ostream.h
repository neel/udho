#ifndef UDHO_NET_OSTREAM_OSTREAM_H
#define UDHO_NET_OSTREAM_OSTREAM_H

#include <string>
#include <udho/utils/string_view.h>
#include <udho/utils/format.h>
#include <boost/beast/http/fields.hpp>
#include <boost/asio/strand.hpp>
#include <udho/net/common.h>
#include <udho/net/ostream/detail/header_writer.h>
#include <udho/net/ostream/detail/buffered_ostream.h>
#include <udho/net/ostream/detail/queued_ostream.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/logging/macros.h>

namespace udho{
namespace net{

struct ostream_view{
private:
    void* _self = nullptr;
    void (*_set)(void*, const boost::beast::http::field&, udho::utils::string_view) = nullptr;
    void (*_write_string)(void*, std::string&&) = nullptr;
    void (*_write_sv)(void*, udho::utils::string_view) = nullptr;
    void (*_write_raw)(void*, const char*, std::size_t, bool) = nullptr;
    void (*_disable_buffering)(void*) = nullptr;
    void (*_finish)(void*) = nullptr;
    void (*_status)(void*, boost::beast::http::status) = nullptr;

    ostream_view(
        void* self,
        void (*st)(void*, const boost::beast::http::field&, udho::utils::string_view),
        void (*ws)(void*, std::string&&),
        void (*wsv)(void*, udho::utils::string_view),
        void (*wr)(void*, const char*, std::size_t, bool),
        void (*db)(void*),
        void (*status)(void*, boost::beast::http::status),
        void (*fn)(void*)
        )
        : _self(self)
        , _set(st)
        , _write_string(ws)
        , _write_sv(wsv)
        , _write_raw(wr)
        , _disable_buffering(db)
        , _status(status)
        , _finish(fn)
    {}

public:
    template <typename OstreamT>
    static ostream_view bind(OstreamT& ostream) {
        return {
            &ostream,
            [](void* s, const boost::beast::http::field& field, udho::utils::string_view value){ static_cast<OstreamT*>(s)->set(field, value); },
            [](void* s, std::string&& x){ static_cast<OstreamT*>(s)->write(std::move(x)); },
            [](void* s, udho::utils::string_view x){ static_cast<OstreamT*>(s)->write(x); },
            [](void* s, const char* p, std::size_t n, bool c){ static_cast<OstreamT*>(s)->write(p, n, c); },
            [](void* s){ static_cast<OstreamT*>(s)->disable_buffering(); },
            [](void* s, boost::beast::http::status t){ static_cast<OstreamT*>(s)->status(t); },
            [](void* s){ static_cast<OstreamT*>(s)->finish(); }
        };
    }

    ostream_view() = delete;
public:
    void set(const boost::beast::http::field& field, udho::utils::string_view value)  { _set(_self, field, value); }
    void write(std::string&& s)               { _write_string(_self, std::move(s)); }
    void write(udho::utils::string_view sv)   { _write_sv(_self, sv); }
    void write(const char* p, std::size_t n, bool c = true)  { _write_raw(_self, p, n, c); }
    void disable_buffering()                  { _disable_buffering(_self); }
    void finish()                             { _finish(_self); }
    void status(boost::beast::http::status t) { _status(_self, t); }

    friend ostream_view& operator<<(ostream_view& ostream, std::string&& value) {
        ostream.write(std::move(value));
        return ostream;
    }

    friend ostream_view& operator<<(ostream_view& ostream, std::string_view value) {
        ostream.write(value);
        return ostream;
    }

    template <std::size_t N>
    friend ostream_view& operator<<(ostream_view& os, const char (&literal)[N]) {
        os.write(literal, N - 1);
        return os;
    }
};

enum class ostream_states {
    buffered,
    switching,
    queued,
    buffered_flushing,
    queued_finishing,
    completed
};

/**
 * @brief Composite ostream selecting between buffered and queued output modes.
 *
 * Default mode: buffered
 * - write() appends to `basic_buffered_ostream`
 * - finish() flushes buffered payload; for chunked it then emits terminal chunk via buffered.finish()
 *
 * After disable_buffering():
 * - headers are flushed
 * - `_buffering` becomes false
 * - buffered payload is flushed
 * - queued stream is resumed after buffered flush completes
 * - subsequent writes go to `basic_queued_ostream` and are pumped to socket
 *
 * Completion semantics:
 * - Buffered mode: completion is delivered from buffered completion callback after finish()
 * - Queued mode: completion is delivered from queued completion callback after queued finish()
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream.
 *
 * @thread_safety Public API methods post onto the strand; safe to call from any thread.
 * The class assumes it outlives all posted handlers (typical Asio lifetime rule).
 *
 * ## State transition
 * @dotfile ostream_fsm.dot
 */
template <typename StreamT>
struct basic_ostream{
    using stream_type               = StreamT;
    using executor_type             = typename stream_type::executor_type;
    using strand_type               = boost::asio::strand<executor_type>;
    using encoding_type             = udho::net::types::transfer_encoding;
    using header_writer_type        = detail::basic_header_writer<StreamT>;
    using buffered_stream_type      = detail::basic_buffered_ostream<StreamT>;
    using queued_stream_type        = detail::basic_queued_ostream<StreamT>;
    using completion_callback_type  = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type     = boost::beast::http::header<false, boost::beast::http::fields>;
    using ostream_type              = basic_ostream<StreamT>;
    using response_type             = boost::beast::http::response<boost::beast::http::empty_body>;

    /**
     * @brief Construct composite ostream.
     * @param stream Underlying async write stream.
     * @param callback Completion callback (final completion).
     * @note callback should have regular boost asio completion callback signature. bytes_written
     *       will only include bytes written for the body of the HTTP response
     *
     * @note The queued stream starts paused, it is resumed only after switching away from buffering; if never resumed then uses buffered stream only
     */
    basic_ostream(stream_type& stream, completion_callback_type&& callback)
        : _stream(stream), _strand(stream.get_executor()), _state(ostream_states::buffered), _header_sealed(false), _buffering(true), _finishing(false)
        , _header_stream(stream, _strand, _response, std::bind(&ostream_type::on_header_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _queued_stream(stream, _strand, _encoding,
                         std::bind(&ostream_type::on_queued_completion,   this, std::placeholders::_1, std::placeholders::_2)
                         )
        , _buffered_stream(stream, _strand, _encoding,
                           std::bind(&ostream_type::on_buffered_flush,      this, std::placeholders::_1, std::placeholders::_2),
                           std::bind(&ostream_type::on_buffered_completion, this, std::placeholders::_1, std::placeholders::_2)
                           )
        , _headers_sent(false), _bytes_written(0), _completion(std::move(callback))
    {
        _queued_stream.pause();
    }

    basic_ostream(const basic_ostream&) = delete;
    basic_ostream(basic_ostream&&) = delete;

    const response_headers_type& headers() const { return _response; }

    void set(const boost::beast::http::field& field, udho::utils::string_view value){
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("headers must be set before the headers are sent to the socket"));
        }
        _response.set(field, value);
    }

    udho::net::types::transfer::encoding encoding() const { return _encoding.encoding(); }

    udho::net::types::transfer::compression compression() const { return _encoding.compression(); }

    void status(boost::beast::http::status st) { _response.result(st); }

    void encoding(udho::net::types::transfer::encoding enc) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("encoding must be set before the headers are sent to the socket"));
        }
        _encoding.encoding(enc);
    }

    void compression(udho::net::types::transfer::compression cmp) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("compression must be set before the headers are sent to the socket"));
        }
        _encoding.compression(cmp);
    }

    ostream_view view () { return ostream_view::bind(*this); }

    template <typename T>
    friend ostream_type& operator<<(ostream_type& ostream, T&& value) {
        ostream.write(std::forward<T>(value));
        return ostream;
    }

    template <std::size_t N>
    friend ostream_type& operator<<(ostream_type& os, const char (&literal)[N]) {
        os.write(literal, N - 1);
        return os;
    }

    /**
     * @brief Permanently disable buffering (switch to queued streaming).
     *
     * This posts onto the strand and calls switch_stream() once.
     * Subsequent calls are ignored.
     */
    void disable_buffering() {
        _header_sealed = true;
        _encoding.encoding(udho::net::types::transfer::encoding::chunked);
        boost::asio::post(_strand, [this](){
            if(!_buffering) return;
            if(_finishing)  return;
            if(_state != ostream_states::buffered) return;
            switch_stream();
        });
    }

public:
    /// @brief Write an ostreamable value to the active output stream.
    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T> && !udho::utils::traits::is_string<T>::value, bool> = true>
    void write(T&& value) {
        boost::asio::post(_strand, [this, value = std::move(value)](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(std::move(value));
            } else {
                _queued_stream.write(std::move(value));
            }
        });
    }

    /// @brief Write owned string to the active output stream.
    void write(std::string&& str) {
        boost::asio::post(_strand, [this, str = std::move(str)](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(std::move(str));
            } else {
                _queued_stream.write(std::move(str));
            }
        });
    }

    /// @brief Write string_view to the active output stream (copied in buffered, borrowed in queued).
    void write(udho::utils::string_view str) {
        boost::asio::post(_strand, [this, str](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(str);
            } else {
                _queued_stream.write(str);
            }
        });
    }

    /**
     * @brief No-copy write in queued mode; buffered mode still copies into internal buffer.
     * @param data Pointer to bytes.
     * @param size Number of bytes.
     *
     * @warning In queued mode, caller must ensure lifetime until async write completion.
     */
    void write(const char* data, std::size_t size, bool copy) {
        if(copy) {
            boost::beast::flat_buffer buffer;
            boost::asio::mutable_buffer mutable_buf = buffer.prepare(size);
            std::memcpy(mutable_buf.data(), data, size);
            buffer.commit(size);

            boost::asio::post(_strand, [this, buff = std::move(buffer)]() mutable {
                if(_finishing) return;
                if(_buffering) {
                    _buffered_stream.write(std::move(buff));
                } else {
                    _queued_stream.write(std::move(buff));
                }
            });
        } else {
            boost::asio::post(_strand, [this, data, size](){
                if(_finishing) return;
                if(_buffering) {
                    _buffered_stream.write(data, size);
                } else {
                    _queued_stream.write(data, size);
                }
            });
        }

    }

    void write(boost::iostreams::mapped_file_source&& mmaped_file) {
        boost::asio::post(_strand, [this, file = std::move(mmaped_file)](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(file);
            } else {
                _queued_stream.write(file);
            }
        });
    }

    /**
     * @brief Finish the response.
     *
     * Semantics:
     * - Marks `_finishing` to reject subsequent writes.
     * - Ensures headers are flushed.
     * - If buffering: flush buffered payload; completion continues in callbacks:
     *     - on_buffered_flush => if finishing && chunked => buffered.finish()
     *     - on_buffered_completion => user completion callback (buffering case)
     * - If queued: queued.finish() which eventually completes (and chunked terminal if needed)
     */
    void finish() {
        _header_sealed = true;
        boost::asio::post(_strand, [this](){
            _finishing = true;
            if(_state == ostream_states::buffered_flushing) return;
            if(_state == ostream_states::queued_finishing)   return;

            if(_state == ostream_states::buffered) {
                // assert(_headers_sent);
                assert(_buffering);
                if(!_headers_sent) {
                    flush_headers(false);
                }
            } else if(_state == ostream_states::queued) {
                assert(_headers_sent);
                assert(!_buffering);
                _queued_stream.finish();
            } else {
                return;
            }
        });
    }

private:

    /**
     * @brief sets the necessary headers and flushes _header_stream
     * @warning must be called on the strand to ensure sequential processing
     */
    void flush_headers(bool chunked) {
        // send Content Length Header [if using buffered stream]

        // ( _buffering &&  chunked )  =>  buffering is true on record; stream being switched to unbuffered -> Transfer-Encoding: Chunked
        // ( _buffering && !chunked )  =>  buffering is true on record; no switching happening              -> Content-Length: N
        // (!_buffering &&  chunked )  =>  buffering is false; implies switching has already happened       -> Meaningless
        // (!_buffering && !chunked )  =>  buffering is false; implies switching has already happened       -> Meaningless

        assert(_buffering);

        if(!chunked) {
            _response.set(boost::beast::http::field::content_length, std::to_string(_buffered_stream.size()));
        } else {
            _response.set(boost::beast::http::field::transfer_encoding, "chunked");
        }
        _header_stream.flush();
    }

    /**
     * @brief Switch from buffered to queued mode.
     *
     * Ordering guarantee (all on same strand):
     * 1) flush headers
     * 2) set `_buffering = false`
     * 3) initiate buffered flush
     *
     * Because queued stream is paused until buffered flush completes,
     * any writes routed to queued during the transition will queue up,
     * but will not reach the socket until resume() is called in on_buffered_flush.
     */
    void switch_stream() {
        std::cout << "states::switching" << std::endl;
        _state = ostream_states::switching;
        // all these calls will happen on the strand and the same strand is used by
        // all write calls, so the order of headers flush, setting buffering, flushing
        // buffered_stream and resuming _queued_stream prohibits existance of unflushed
        // data in the buffered stream, transmission of data from queued_stream until
        // _buffered_stream flush completes
        assert(_buffering);
        // call flush on all streams, no need to wait for them to finish.
        // we are good as soon as these calls are queued to the strand
        assert(!_headers_sent);
        flush_headers(true);
        // _buffering is atomic, so other threads calling write and thus checking
        // write is not a problem.
        _buffering = false;
        // subsequent calls to write will now use queued_stream, however write only
        // posts to the strand, if some other thread calls write now, it will post
        // a _queued_stream.write on strand and _queued_stream is paused

        // see you in on_header_completion
    }

    /// @brief Header completion handler: marks headers sent and accumulates bytes.
    void on_header_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _headers_sent = true;
        _bytes_written += bytes_written;
        std::cout << "on_header_completion" << std::endl;

        namespace params = udho::logging::params;
        UDHO_LOG_DEBUG("udho::net::ostream", "Response Headers flushed", params::socket_id(udho::utils::misc::native_handle(_stream)));

        if(ec) on_error(ec);
        else {
            if(_state == ostream_states::switching) {
                assert(!_buffering);
                // _finishing may or may not be true -> so don't make any decision based on that yet
            } else {
                assert(_state == ostream_states::buffered);
                assert(_buffering);
                assert(_finishing);
            }
            _buffered_stream.async_flush();
        }
    }

    /**
     * @brief Buffered flush handler (payload flush completed).
     *
     * - If still buffering and finish() has been requested and encoding is chunked: emit terminal chunk.
     * - If switched to queued: resume queued pump so queued writes drain after buffered flush.
     */
    void on_buffered_flush(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        std::cout << "on_buffered_flush" << std::endl;
        if(ec) on_error(ec);
        else {
            if(_state == ostream_states::buffered) {
                assert(_buffering);
                assert(_finishing);
                std::cout << "states::buffered_flushing" << std::endl;
                _state = ostream_states::buffered_flushing;
                if(_finishing){
                    _buffered_stream.finish();
                }
            } else {
                assert(_state == ostream_states::switching);
                assert(!_buffering);
                _queued_stream.resume();
                std::cout << "states::queued" << std::endl;
                _state = ostream_states::queued;
                // Any intermediate writes routed to the _queued_stream now gets
                // pumped out to the socket
                if(_finishing) {
                    finish();
                }
            }
        }
    }

    /// @brief Buffered completion handler (after finish if needed).
    void on_buffered_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        std::cout << "on_buffered_completion" << std::endl;
        if(ec) on_error(ec);
        else {
            assert(_headers_sent);
            if(_buffering) {
                std::cout << "states::completed" << std::endl;
                _state = ostream_states::completed;
                if(_completion) {
                    _completion(ec, _bytes_written);
                }
            }
        }
    }

    /// @brief Queued completion handler (called after queued.finish drains and terminal sent if needed).
    void on_queued_completion(boost::system::error_code ec, std::size_t bytes_written) {
        std::cout << "states::queued_finishing" << std::endl;
        _state = ostream_states::queued_finishing;
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            assert(_headers_sent);
            std::cout << "states::completed" << std::endl;
            _state = ostream_states::completed;
            if(_completion) {
                _completion(ec, _bytes_written);
            }
        }
    }

    /// @brief Common error path: forward error to user completion callback.
    void on_error(boost::system::error_code ec) {
        _completion(ec, _bytes_written);
    }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after the response has been flushed to the socket and the
     *          completion callback has been called
     */
    void reset() {
        assert(_header_sealed);

        _header_stream.reset();
        _buffered_stream.reset();
        if(!_buffering) {
            _queued_stream.reset();
        }

        _response.clear();

        _buffering      = true;
        _headers_sent   = false;
        _bytes_written  = 0;
        _finishing      = false;
        _header_sealed  = false;

        _state          = ostream_states::buffered;

        _encoding.encoding(udho::net::types::transfer::encoding::plain);
    }

private:
    stream_type&                 _stream;
    strand_type                  _strand;
    response_type                _response;
    encoding_type                _encoding;
private:
    header_writer_type           _header_stream;
    queued_stream_type           _queued_stream;
    buffered_stream_type         _buffered_stream;
private:
    ostream_states               _state;
    std::atomic_bool             _header_sealed;
    bool                         _buffering;
    bool                         _headers_sent;
    std::size_t                  _bytes_written;
    bool                         _finishing;
private:
    completion_callback_type     _completion;
};

using tcp_ostream  = basic_ostream<udho::net::types::socket>;
using test_ostream = basic_ostream<boost::beast::test::stream>;

}
}

#endif // UDHO_NET_OSTREAM_OSTREAM_H
