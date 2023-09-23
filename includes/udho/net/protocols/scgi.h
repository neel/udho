#ifndef UDHO_NET_PROTOCOL_SCGI_H
#define UDHO_NET_PROTOCOL_SCGI_H

#include <iterator>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <boost/utility/string_view.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace udho{
namespace net{
namespace protocols{

template <typename StreamT>
struct scgi_reader: public std::enable_shared_from_this<scgi_reader<StreamT>>{
    using handler_type     = std::function<void (boost::system::error_code, std::size_t)>;
    using field_map_type   = std::map<std::string, boost::beast::http::field>;
    using stream_type      = StreamT;

    inline explicit scgi_reader(types::headers::request& request, stream_type& stream): _request(request), _stream(stream) {
        _field_map = {
            {"HTTP-USER-AGENT",     boost::beast::http::field::user_agent},
            {"HTTP-CONNECTION",     boost::beast::http::field::connection},
            {"HTTP-HOST",           boost::beast::http::field::host},
            {"HTTP-CACHE-CONTROL",  boost::beast::http::field::cache_control},
            {"DOCUMENT-URI",        boost::beast::http::field::uri},
            {"HTTP-ACCEPT",         boost::beast::http::field::accept}
        };
    }

    template <typename Handler>
    void start(Handler&& handler){
        std::cout << "started reading" << std::endl;
        boost::asio::async_read_until(
            _stream, _buffer, "\0,",
            std::bind(&scgi_reader::finished, std::enable_shared_from_this<scgi_reader<StreamT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
        _handler = std::move(handler);
    }
    private:
        void finished(boost::system::error_code ec, std::size_t bytes_transferred){
            if(!ec){
                parse();
            }
            std::cout << "finished" << std::endl;
            _handler(ec, bytes_transferred);
        }
        void parse(){
            std::istream stream(&_buffer);
            std::string word, last_key;
            bool is_key = true;
            boost::beast::http::field field;
            std::getline(stream, word, ':');
            while (std::getline(stream, word, '\0')) {
                if(is_key){
                    last_key = (boost::beast::http::string_to_field(word) == boost::beast::http::field::unknown) ? boost::replace_all_copy(word, "_", "-") : word;
                    field = boost::beast::http::string_to_field(last_key);
                }else{
                    bool known_field = (field != boost::beast::http::field::unknown);
                    if(!known_field){
                        std::string key_upper = boost::algorithm::to_upper_copy(last_key);
                        if(key_upper == "REQUEST-URI"){
                            _request.target(word);
                        }else if(key_upper == "REQUEST-METHOD"){
                            _request.method_string(word);
                        }else{
                            auto it = _field_map.find(key_upper);
                            if(it != _field_map.end()){
                                _request.insert(it->second, word);
                            }else{
                                std::cout << "unknown field: " << last_key << " value: " << word << std::endl;
                            }
                        }
                    }else{
                        _request.insert(field, word);
                    }
                }
                is_key = !is_key;
            }
        }
    private:
        udho::net::types::headers::request& _request;
        boost::asio::streambuf              _buffer;
        handler_type                        _handler;
        stream_type&                        _stream;
        field_map_type                      _field_map;
};


}
}
}


#endif // UDHO_NET_PROTOCOL_SCGI_H

