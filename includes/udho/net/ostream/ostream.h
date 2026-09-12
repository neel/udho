#ifndef UDHO_NET_OSTREAM_OSTREAM_H
#define UDHO_NET_OSTREAM_OSTREAM_H

#include <string>
#include <chrono>
#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
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
#include <udho/exceptions/exceptions.h>

namespace udho{
namespace net{

template <typename StreamT>
struct basic_ostream_timeout {
    inline static constexpr auto value = std::chrono::minutes{1};
};

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Non-owning, type-erased view of an HTTP response output stream.
 *
 * This view exposes the response metadata, body-writing, and completion
 * operations needed by code that should not depend on the concrete stream
 * type used by basic_ostream. bind() stores the address of the concrete stream
 * and a small forwarding table; it does not take ownership of that stream.
 *
 * @note The bound stream must outlive this view and all operations initiated
 *       through it.
 */
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

    /**
     * @brief Construct a view from an erased stream instance and its forwarding functions.
     * @param self Address of the concrete stream instance.
     * @param st Response-header setter.
     * @param ws Owned-string writer.
     * @param wsv String-view writer.
     * @param wr Raw byte-range writer.
     * @param db Buffering-control function.
     * @param status Response-status setter.
     * @param fn Response-finishing function.
     */
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
    /**
     * @brief Create a type-erased view of a compatible output stream.
     * @tparam OstreamT Concrete stream type providing the operations exposed by ostream_view.
     * @param ostream Stream instance to expose through the view.
     * @return A non-owning view bound to @p ostream.
     * @note @p ostream must outlive the returned view and its pending operations.
     */
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

    /// @brief Disabled; an ostream_view must be created by binding a concrete stream.
    ostream_view() = delete;
public:
    /**
     * @brief Set a response header on the bound stream.
     * @param field Header field to set.
     * @param value Header value.
     */
    void set(const boost::beast::http::field& field, udho::utils::string_view value)  { _set(_self, field, value); }

    /**
     * @brief Write an owned string to the bound stream.
     * @param s String whose contents are transferred to the stream operation.
     */
    void write(std::string&& s)               { _write_string(_self, std::move(s)); }

    /**
     * @brief Write a string view to the bound stream.
     * @param sv Character range to write.
     * @warning In queued mode the referenced characters must remain valid until
     *          the asynchronous write completes.
     */
    void write(udho::utils::string_view sv)   { _write_sv(_self, sv); }

    /**
     * @brief Write a raw byte range to the bound stream.
     * @param p Pointer to the first byte.
     * @param n Number of bytes to write.
     * @param c Whether to copy the bytes before queueing the write.
     * @warning When @p c is false, the byte range must remain valid until the
     *          asynchronous write completes.
     */
    void write(const char* p, std::size_t n, bool c = true)  { _write_raw(_self, p, n, c); }

    /// @brief Switch the bound stream from buffered output to queued streaming.
    void disable_buffering()                  { _disable_buffering(_self); }

    /// @brief Finish the response after all accepted body data has been written.
    void finish()                             { _finish(_self); }

    /**
     * @brief Set the HTTP response status on the bound stream.
     * @param t Status to assign to the response.
     */
    void status(boost::beast::http::status t) { _status(_self, t); }

    /**
     * @brief Write an owned string through a type-erased stream view.
     * @param ostream View receiving the string.
     * @param value String whose contents are transferred to the stream operation.
     * @return @p ostream, allowing insertion operations to be chained.
     */
    friend ostream_view& operator<<(ostream_view& ostream, std::string&& value) {
        ostream.write(std::move(value));
        return ostream;
    }

    /**
     * @brief Write a string view through a type-erased stream view.
     * @param ostream View receiving the character range.
     * @param value Character range to write.
     * @return @p ostream, allowing insertion operations to be chained.
     * @warning In queued mode the referenced characters must remain valid until
     *          the asynchronous write completes.
     */
    friend ostream_view& operator<<(ostream_view& ostream, std::string_view value) {
        ostream.write(value);
        return ostream;
    }

    /**
     * @brief Write a string literal without its terminating null character.
     * @tparam N Size of the literal array, including its null terminator.
     * @param os View receiving the literal.
     * @param literal Literal to write.
     * @return @p os, allowing insertion operations to be chained.
     */
    template <std::size_t N>
    friend ostream_view& operator<<(ostream_view& os, const char (&literal)[N]) {
        os.write(literal, N - 1);
        return os;
    }
};

/// @brief States of the buffered-to-queued response output lifecycle.
enum class ostream_states {
    buffered,           ///< Body data is accumulating in the in-memory buffer.
    switching,          ///< Headers and buffered data are being flushed before queued output starts.
    queued,             ///< Body data is being written through the ordered output queue.
    buffered_flushing,  ///< The final buffered response is being flushed.
    queued_finishing,   ///< Queued output is draining and completing the response.
    completed           ///< The response and its completion callback have finished.
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
 * ### Thread safety
 *
 * Public API methods post onto the strand; safe to call from any thread.
 * The class assumes it outlives all posted handlers (typical Asio lifetime rule).
 *
 * ## State transition
 * @dotfile ostream_fsm.dot
 */
template <typename StreamT>
struct basic_ostream{
    using stream_type               = StreamT;
    using ostream_type              = basic_ostream<StreamT>;
    using executor_type             = typename stream_type::executor_type;
    using strand_type               = boost::asio::strand<executor_type>;
    using encoding_type             = udho::net::types::transfer_encoding;
    using header_writer_type        = detail::basic_header_writer<StreamT>;
    using buffered_stream_type      = detail::basic_buffered_ostream<StreamT>;
    using queued_stream_type        = detail::basic_queued_ostream<StreamT>;
    using completion_callback_type  = std::function<void (boost::system::error_code, std::size_t)>;
    using exception_callback_type   = std::function<void (ostream_type&)>;
    using response_headers_type     = boost::beast::http::header<false, boost::beast::http::fields>;
    using response_type             = boost::beast::http::response<boost::beast::http::empty_body>;
    using traced_exception_type     = udho::exceptions::captured;
    using opt_traced_exception_type = std::optional<traced_exception_type>;
    using timer_type                = boost::asio::steady_timer;
    using time_point_type           = typename timer_type::time_point;

    /**
     * @brief Construct composite ostream.
     * @param stream Underlying async write stream.
     * @param callback Completion callback (final completion).
     * @param ex_callback Exception handler callback (must call finish() after processing exception).
     * @note callback should have regular boost asio completion callback signature. bytes_written
     *       will only include bytes written for the body of the HTTP response
     * @note ex_callback gets a mutable reference to the ostream as argument. The ex_callback should
     *       write exception and related information to the ostream and call finish() function.
     * @note The queued stream starts paused, it is resumed only after switching away from buffering; if never resumed then uses buffered stream only
     */
    basic_ostream(stream_type& stream, completion_callback_type&& callback, exception_callback_type&& ex_callback)
        : _stream(stream), _strand(stream.get_executor()), _timer(_strand), _prepared(false), _wait_outstanding(false), _state(ostream_states::buffered), _header_sealed(false), _buffering(true), _finishing(false)
        , _header_stream(stream, _strand, _response, std::bind(&ostream_type::on_header_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _queued_stream(stream, _strand, _encoding, std::bind(&ostream_type::on_queued_completion,   this, std::placeholders::_1, std::placeholders::_2))
        , _buffered_stream(stream, _strand, _encoding,
                           std::bind(&ostream_type::on_buffered_flush,      this, std::placeholders::_1, std::placeholders::_2),
                           std::bind(&ostream_type::on_buffered_completion, this, std::placeholders::_1, std::placeholders::_2)
                        )
        , _headers_sent(false), _bytes_written(0), _completion(std::move(callback)), _capex_handler(std::move(ex_callback)), _completion_status{{}, 0}
    {
        _queued_stream.pause();
    }

    /// @brief Copy construction is disabled because asynchronous handlers refer to this instance.
    basic_ostream(const basic_ostream&) = delete;

    /// @brief Move construction is disabled because asynchronous handlers refer to this instance.
    basic_ostream(basic_ostream&&) = delete;

    /**
     * @brief Access the current HTTP response headers.
     * @return Read-only response headers, including the current status.
     */
    const response_headers_type& headers() const { return _response; }

    /**
     * @brief Set an HTTP response header before the headers are sealed.
     * @param field Header field to set.
     * @param value Header value.
     * @throws std::runtime_error if the response headers have already been sealed.
     */
    void set(const boost::beast::http::field& field, udho::utils::string_view value){
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("headers must be set before the headers are sent to the socket"));
        }
        _response.set(field, value);
    }

    /**
     * @brief Get the configured transfer encoding.
     * @return Current transfer encoding.
     */
    udho::net::types::transfer::encoding encoding() const { return _encoding.encoding(); }

    /**
     * @brief Get the configured transfer compression.
     * @return Current transfer compression.
     */
    udho::net::types::transfer::compression compression() const { return _encoding.compression(); }

    /**
     * @brief Set the HTTP response status.
     * @param st Status to assign to the response.
     */
    void status(boost::beast::http::status st) { _response.result(st); }

    /**
     * @brief Set the transfer encoding before the headers are sealed.
     * @param enc Transfer encoding to use.
     * @throws std::runtime_error if the response headers have already been sealed.
     */
    void encoding(udho::net::types::transfer::encoding enc) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("encoding must be set before the headers are sent to the socket"));
        }
        _encoding.encoding(enc);
    }

    /**
     * @brief Set the transfer compression before the headers are sealed.
     * @param cmp Transfer compression to use.
     * @throws std::runtime_error if the response headers have already been sealed.
     */
    void compression(udho::net::types::transfer::compression cmp) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("compression must be set before the headers are sent to the socket"));
        }
        _encoding.compression(cmp);
    }

    /**
     * @brief Obtain a non-owning, type-erased view of this stream.
     * @return A view forwarding operations to this stream.
     * @note This stream must outlive the returned view and its pending operations.
     */
    ostream_view view () { return ostream_view::bind(*this); }

    /**
     * @brief Write a value through the response output stream.
     * @tparam T Value type accepted by a corresponding write() overload.
     * @param ostream Stream receiving the value.
     * @param value Value forwarded to the active buffered or queued stream.
     * @return @p ostream, allowing insertion operations to be chained.
     */
    template <typename T>
    friend ostream_type& operator<<(ostream_type& ostream, T&& value) {
        ostream.write(std::forward<T>(value));
        return ostream;
    }

    /**
     * @brief Write a string literal without its terminating null character.
     * @tparam N Size of the literal array, including its null terminator.
     * @param os Stream receiving the literal.
     * @param literal Literal to write.
     * @return @p os, allowing insertion operations to be chained.
     */
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
            if(_capex.has_value()) return;
            _extend();
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
            if(_capex.has_value()) return;
            _extend();
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
            if(_capex.has_value()) return;
            _extend();
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
     * @param copy Whether to copy the bytes before queueing the write.
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
                if(_capex.has_value()) return;
                _extend();
                if(_buffering) {
                    _buffered_stream.write(std::move(buff));
                } else {
                    _queued_stream.write(std::move(buff));
                }
            });
        } else {
            boost::asio::post(_strand, [this, data, size](){
                if(_finishing) return;
                if(_capex.has_value()) return;
                _extend();
                if(_buffering) {
                    _buffered_stream.write(data, size);
                } else {
                    _queued_stream.write(data, size);
                }
            });
        }

    }

    /**
     * @brief Submit a memory-mapped file to the active output stream.
     * @param mmaped_file Open mapped file whose ownership is transferred to the queued operation.
     */
    void write(boost::iostreams::mapped_file_source&& mmaped_file) {
        boost::asio::post(_strand, [this, file = std::move(mmaped_file)](){
            if(_finishing) return;
            if(_capex.has_value()) return;
            _extend();
            if(_buffering) {
                _buffered_stream.write(file);
            } else {
                _queued_stream.write(file);
            }
        });
    }

    /**
     * @brief Write error-page content while a captured exception is active.
     *
     * Unlike @ref write, this function deliberately permits output while the
     * stream holds a captured exception. It is intended exclusively for the
     * framework's error renderer, which must be able to finish an error
     * response after ordinary application writes have been disabled.
     *
     * @param str Owned error-page content to write.
     */
    void _write(std::string&& str) {
        boost::asio::post(_strand, [this, str = std::move(str)](){
            if(_finishing) return;
            _extend();
            if(_buffering) {
                _buffered_stream.write(std::move(str));
            } else {
                _queued_stream.write(std::move(str));
            }
        });
    }

    /**
     * @brief Check whether response metadata can no longer be changed.
     * @return true after streaming or response completion has sealed the headers.
     */
    bool headers_sealed() const {
        return _header_sealed;
    }

    /**
     * @brief Discard body data accumulated while the stream is still buffering.
     *
     * The request is posted to the stream strand. It has no effect after the
     * stream has switched to queued output, or while the buffered stream is
     * already flushing.
     */
    void try_clear() {
        boost::asio::post(_strand, [this](){
            if(_buffering) {
                _buffered_stream.clear();
            }
        });
    }

    /**
     * @brief pass a captured exception to the ostream
     * This leads to a call to the exception handler set to the ostream from the
     * constructor. The exception handler may write the exception and related
     * information to the ostream. Afterwards the callback **MUST** call `finish()`
     * to flush the output to the socket including both previous output as well as
     * exception related output.
     *
     * @param traced_exception may be captured using udho::exceptions::captured
     * @pre finish() has not yet been called
     * @warning the exception handler must call the finish() method
     * @note If headers have not been flushed then set it response status as 500
     *       Internal Server error. Contents that have already been written to the
     *       buffered or queued stream will still be written.
     * @note Any contents or exceptions added after calling this function will be ignored.
     * @post All write() calls will be ignored, after an exception is passed to it.
     * @note if ostream was using queued_stream then whatever was written to the stream
     *       before setting the exception will be written to the HTTP response
     */
    void exception(traced_exception_type&& traced_exception) {
        assert(!_finishing);
        boost::asio::post(_strand, [this, ex = std::move(traced_exception)]() mutable {
            _exception(std::move(ex));
        });
    }

    /**
     * @brief Check whether a captured exception has been assigned to the stream.
     * @return true when exception(traced_exception_type&&) has stored an exception.
     */
    bool has_exception() const { return _capex.has_value(); }

    /**
     * @brief Access the captured exception assigned to the stream.
     * @return The stored captured exception.
     * @pre has_exception() is true.
     */
    const traced_exception_type& exception() const {
        assert(has_exception());
        return _capex.value();
    }
private:
    void _exception(traced_exception_type&& traced_exception) {
        if(_finishing || _capex.has_value()) return;

        if(!_headers_sent) {
            status(boost::beast::http::status::internal_server_error);
        }
        _capex = std::move(traced_exception);
        _capex_handler(*this);
    }

public:

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
            assert(_prepared);

            _finishing = true;

            stop();

            if(_state == ostream_states::buffered_flushing) return;
            if(_state == ostream_states::queued_finishing)   return;

            if(_state == ostream_states::buffered) {
                assert(_buffering);
                if(!_headers_sent) {
                    flush_headers(false);
                    // once the feaders are flushed asynchronously on_header_completion() will call _buffered_stream.async_flush()
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
        std::cout << "on_header_completion " << ec.message() << std::endl;

        namespace params = udho::logging::params;

        if(ec) {
            std::string err_msg = ec.message();
            UDHO_LOG_ERROR("udho::net::ostream", udho::utils::format("Error while flushing response Headers {}", err_msg), params::socket_id(udho::utils::misc::native_handle(_stream)));

            on_error(ec);
        } else {
            UDHO_LOG_DEBUG("udho::net::ostream", "Response Headers flushed", params::socket_id(udho::utils::misc::native_handle(_stream)));

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
                completion(ec, _bytes_written);
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
            completion(ec, _bytes_written);
        }
    }

    /// @brief Common error path: forward error to user completion callback.
    void on_error(boost::system::error_code ec) {
        stop();
        completion(ec, _bytes_written);
    }

    /**
     * @brief ostream content write completion handler
     * @param ec
     * @param bytes_written
     */
    void completion(boost::system::error_code ec, std::size_t bytes_written) {
        _completion_status = std::make_pair(ec, bytes_written);
        reset(ec);
    }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after the response has been flushed to the socket and the
     *          completion callback has been called
     *
     * @param ec error code if reset is called after some error occured
     */
    void reset(boost::system::error_code ec = {}) {
        assert(_header_sealed);

        _header_stream.reset(ec);
        _buffered_stream.reset(ec);
        if(!_buffering) {
            _queued_stream.reset(ec);
        }

        _response.clear();
        _response.result(boost::beast::http::status::ok);

        _buffering      = true;
        _headers_sent   = false;
        _bytes_written  = 0;
        _finishing      = false;
        _header_sealed  = false;

        _state          = ostream_states::buffered;

        _encoding.encoding(udho::net::types::transfer::encoding::plain);
        _capex.reset();

        _prepared = false;
        stop();
    }

public:
    /**
     * @brief begin idle timer
     *
     * The operation is idempotent for the current response. The prepared state is
     * cleared by reset() after response completion.
     */
    void prepare() {
        boost::asio::post(_strand, [this]() {
            if(_prepared) return;

            _prepared    = true;
            _expiry_time = timer_type::clock_type::now() + idle_time_duration;

            if(!_wait_outstanding) {
                _start();
            }
        });
    }
private:
    void _extend() {
        if(!_prepared || _finishing) return;

        const auto now = timer_type::clock_type::now();
        if(now >= _expiry_time) return;

        _expiry_time = now + idle_time_duration;
    }

    void _start() {
        assert(_prepared);
        assert(!_finishing);
        assert(!_wait_outstanding);

        _timer.expires_at(_expiry_time);
        _wait_outstanding = true;
        _timer.async_wait([this](boost::system::error_code ec) {
            timeout(ec);
        });
    }

    /**
     * @brief stops the timer
     * @param ec content output related error code
     */
    void stop() {
        std::size_t canceled_op_count = _timer.cancel();
        if(!_prepared && !canceled_op_count && !_wait_outstanding) {
            boost::asio::post(_strand, [this](){
                cleanup();
            });
        }
    }


    void timeout(boost::system::error_code ec) {
        assert(_wait_outstanding);
        _wait_outstanding = false;

        const bool aborted  = (ec == boost::asio::error::operation_aborted);
        const bool error    = (ec && !aborted); // timer error
        const auto now      = timer_type::clock_type::now();
        const bool stopping = (!_prepared || _finishing);
        const bool early    = (now < _expiry_time);

        if(error) {
            on_error(ec);
            return;
        } else if(early || aborted) {
            assert(!error);
            assert(!ec || aborted);

            if(!stopping) {
                _start();
                return;
            }
            // else cleanup
        } else if(!stopping) {
            assert(!error);
            assert(!early && !aborted);

            // !early && !aborted && !error -> natural timeout called after expiry time

             _exception(udho::exceptions::captured::propagate(udho::http::error(boost::beast::http::status::service_unavailable, "udho::net::ostream timed out waiting for write", udho::http::error::options::close)));
            return;
        }

        assert(!error);
        assert(stopping);
        assert(!_wait_outstanding);
        // (early || aborted) may or may not be true;

        cleanup();
    }

    void cleanup() {
        assert(!_wait_outstanding);
        if(!_prepared) {
            // reset has been called
            // resets sets _finishing to false
            assert(!_finishing);
            _retire();
        }
    }

    void _retire() {
        if(_completion) {
            boost::system::error_code ec = _completion_status.first;
            std::size_t bytes_written    = _completion_status.second;
            _completion_status           = std::make_pair(boost::system::error_code{}, 0);

            _completion(ec, bytes_written);
        }
    }
private:
    stream_type&                 _stream;
    strand_type                  _strand;
    response_type                _response;
    encoding_type                _encoding;
private:
    timer_type                   _timer;
    bool                         _prepared;
    time_point_type              _expiry_time;
    bool                         _wait_outstanding;
    inline static constexpr auto idle_time_duration = basic_ostream_timeout<stream_type>::value;
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
    using completion_status = std::pair<boost::system::error_code, std::size_t>;

    completion_callback_type     _completion;
    completion_status            _completion_status;
private:
    opt_traced_exception_type    _capex;
    exception_callback_type      _capex_handler;
};

/// @brief HTTP response output stream backed by the project's TCP socket type.
using tcp_ostream  = basic_ostream<udho::net::types::socket>;

/// @brief HTTP response output stream backed by a Boost.Beast test stream.
using test_ostream = basic_ostream<boost::beast::test::stream>;

/** @} */

}
}

#endif // UDHO_NET_OSTREAM_OSTREAM_H
