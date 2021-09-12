#include "udho/page.h"
#include <boost/format.hpp>


std::string udho::exceptions::visual::block::html() const {
    std::stringstream stream;
    stream << "<div ";
    if(!_id.empty()){
        stream << "id = '" << _id << "' ";
    }
    if(!_classes.empty()){
        stream << "class = '";
        for(const std::string& class_name: _classes){
            stream << class_name << " ";
        }
        stream << "'";
    }
    stream << ">";
    stream << _content;
    stream << "</div>";
    return stream.str();
}

std::string udho::exceptions::visual::page::html() const {
    static std::string css = R"(
        body{
            margin: 0px;
        }
        .resource-path{
            background-color: #d28a93;
            color: white;
            padding-left: 8px;
            padding-top: 2px;
            padding-right: 8px;
            padding-bottom: 2px;
            border-radius: 8px;
        }
        .wrapper{
            margin-left: auto;
            margin-right: auto;
            min-width: 500px;
            width: 80%%;
            max-width: 800px;
            font-family: monospace;
        }
        .header{
            font-size: 40px;
            width: 100%%;
            float: left;
            position: relative;
        }
        .block{
            width: 100%%;
            text-align: center;
            margin-top: 20px;
            margin-bottom: 20px;
        }
        .main{
            width: 100%%;
        }
        .udho-module{
            display: table-row;
        }
        .column{
            display: table-cell;
            padding-right: 22px;
            text-overflow: ellipsis;
            white-space: nowrap;
        }
        .udho-module-method{

        }
        .udho-module-pattern{

        }
        .udho-module-fptr{

        }
        .udho-module-compositor{

        }
        .udho-application-name{
            
        }
        .udho-application-path{
            
        }
        .udho-application-fptr{
            
        }
        .udho-application{
            display: table-caption;
            position: relative;
            float: left;
            padding: 4px 0px 10px 10px;
            background: #FFFFFF;
            -webkit-box-shadow: 2px 1px 11px 2px rgba(240,232,232,1);
            -moz-box-shadow: 2px 1px 11px 2px rgba(240,232,232,1);
            box-shadow: 2px 1px 11px 2px rgba(240,232,232,1);
            margin-top: 7px;
            -webkit-border-radius: 9px;
            -moz-border-radius: 9px;
            border-radius: 9px;
            width: 100%%;
            padding-left: 8px;
            padding-right: 8px;
        }
        .udho-application-heading{
            border-bottom: 2px solid #d28a93;
            padding-bottom: 3px;
        }
        .udho-application-overloads{
            margin-top: 2px;
        }
    )";
    std::stringstream stream;
    stream << "<html>"
           <<   "<head>"
           <<       "<title>" << (int) _status << " Error " << "</title>"
           <<       "<style>" << css << "</style>"
           <<   "</head>"
           <<   "<body>"
           <<       "<div class='status'>" << (int) _status << " " << _status << "</div>"
           <<       "<div class='heading'>" << _heading << "</div>"
           <<       "<div class='blocks'>";
    for(const block& blk: _blocks){
        stream << "<div class='block'>" << blk.html() << "</div>";
    }
    stream <<       "</div>"
           <<   "</body>"
           << "</html>";
    return stream.str();
}

udho::exceptions::http_error::http_error(boost::beast::http::status status, const std::string& message): _status(status), _message(message){
    _err_str = (boost::format("%1% Error") % _status).str();
}

void udho::exceptions::http_error::add_header(boost::beast::http::field key, const std::string& value){
    _headers.insert(key, value);
}

void udho::exceptions::http_error::redirect(const std::string& url){
    add_header(boost::beast::http::field::location, url);
}

const char* udho::exceptions::http_error::what() const noexcept{
    return _err_str.c_str();
}

std::string udho::exceptions::http_error::page(const std::string& target, std::string content) const{
    std::string message = (boost::format("%1% Error while accessing <span class='resource-path'>%2%</span>") % _status % target).str();
    if(!_message.empty()){
        message += (boost::format("<div class='error-message'>%1%</div>") % _message).str();
    }
    visual::page p(_status, message);
    if(!content.empty()){
        visual::block main(content, "main");
        p.add_block(main);
    }
    return p.html();
}

boost::beast::http::status udho::exceptions::http_error::result() const{
    return _status;
}

udho::exceptions::http_error udho::exceptions::http_redirection(const std::string& location, boost::beast::http::status status){
    udho::exceptions::http_error error(status);
    error.redirect(location);
    return error;
}

