#ifndef UDHO_WWW_COMPONENTS_HANDLER_H
#define UDHO_WWW_COMPONENTS_HANDLER_H

#include <map>
#include <functional>
#include <udho/utils/format.h>
#include <udho/www/features.h>
#include <udho/net/ostream.h>
#include <udho/url/summary.h>
#include <udho/manifold/portal.h>

namespace udho{
namespace www{

namespace components{

template <typename StreamT>
struct basic_handler{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "handler";
    using params      = udho::manifold::params<>;

    using stream_type   = StreamT;
    using ostream_type  = udho::net::basic_ostream<stream_type>;

    struct responder{
        using callback_type = std::function<void (boost::system::error_code, std::size_t)>;

        template <typename F>
        responder(stream_type& stream, F&& callback)
            : _ostream(stream, std::bind(&responder::on_finish, this, std::placeholders::_1, std::placeholders::_2))
            , _callback(callback_type{std::move(callback)})
        {}
        responder(const responder&) = delete;
        responder(responder&&) = delete;

        ostream_type& ostream() { return _ostream; }
    private:
        void on_finish(boost::system::error_code ec, std::size_t bytes_written) {
            _callback(ec, bytes_written);
            _ostream.reset();
        }
    private:
        ostream_type  _ostream;
        callback_type _callback;
    };

    using collection_type = std::map<std::size_t, responder>;

    basic_handler(const udho::url::summary::router& summary): _summary(summary) {}

    template <typename F>
    ostream_type& add(std::size_t id, stream_type& stream, F&& callback){
        auto responder_it = _responders.find(id);
        if(responder_it != _responders.end()) {
            assert(responder_it->first == id);
            return responder_it->second.ostream();
        }

        bool success = false;
        std::tie(responder_it, success) = _responders.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(stream, std::forward<F>(callback))
        );
        assert(success);
        assert(responder_it->first == id);
        return responder_it->second.ostream();
    }

    const udho::url::summary::router& summary() const { return _summary; }

private:
    collection_type _responders;
    udho::url::summary::router _summary;
};

} // components
} // www

namespace manifold{

template <typename StreamT, typename JournalT>
struct accessor<udho::www::components::basic_handler<StreamT>, JournalT>: basic_accessor<udho::www::components::basic_handler<StreamT>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::basic_handler<StreamT>, JournalT>;
    using component_type        = udho::www::components::basic_handler<StreamT>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;

    const udho::url::summary::router& routes() const {
        return basic_accessor_type::component().summary();
    }

    const udho::url::summary::mount_point& route(const std::string& name) const {
        return routes()[name];
    }
    template <typename Char, Char... C>
    const udho::url::summary::mount_point& route(udho::hazo::string::str<Char, C...>&& hstr) const {
        return route(hstr.str());
    }

};


} // manifold
} // udho

#endif // UDHO_WWW_COMPONENTS_HANDLER_H
