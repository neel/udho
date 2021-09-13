#include "udho/page.h"
#include <boost/format.hpp>


std::string udho::exceptions::visual::block::html() const {
    std::stringstream stream;
    stream << "<div class='block ";
    if(!_classes.empty())
        for(const std::string& class_name: _classes){
            stream << class_name << " ";
        }
    stream << "'";
    if(!_id.empty())
        stream << "id = '" << _id << "' ";
    stream << ">";

    stream <<   "<h4 class='name'>" << _name << "</h4>";
    stream <<   "<div class='content'>";
    stream <<       _content;
    stream <<   "</div>";
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
            width: 80%;
            max-width: 800px;
            font-family: monospace;
            flex-direction: column;
            display: flex;
            flex-wrap: nowrap;
            align-content: stretch;
            justify-content: space-evenly;
            align-items: stretch;
        }
        .status{
            font-size: 40px;
            margin: 15px 0px 2px 0px;
        }
        .heading{
            font-size: 15px;
        }
        .main{
            margin-top: 10px;
            border-top: 2px solid #888;
        }
        .block{
            width: 100%;
            text-align: center;
            margin-top: 20px;
            margin-bottom: 20px;
        }
        .block .name{
            text-transform: capitalize;
            text-align: left;
            margin-top: 2px;
            margin-bottom: 10px;
        }
        .block .content{
            display: flex;
            flex-direction: column;
            align-content: stretch;
            justify-content: space-evenly;
            align-items: stretch;
            padding-left: 8px;
            border-left: 10px solid #ededed;
        }
        .udho-module{
            display: table-row;
        }
        .routes{
            display: table;
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
            display: flex;
            padding: 4px 0px 10px 10px;
            -webkit-box-shadow: 2px 1px 11px 2px rgb(240 232 232);
            -moz-box-shadow: 2px 1px 11px 2px rgba(240,232,232,1);
            box-shadow: 2px 1px 11px 2px rgb(240 232 232);
            margin-top: 7px;
            -webkit-border-radius: 9px;
            -moz-border-radius: 9px;
            border-radius: 9px;
            padding-left: 8px;
            padding-right: 8px;
            flex-direction: column;
            flex-wrap: nowrap;
            align-content: stretch;
            justify-content: space-evenly;
            align-items: stretch;
        }
        .udho-application-heading{
            border-bottom: 2px solid #d28a93;
            padding-bottom: 3px;
        }
        .udho-application-overloads{
            margin-top: 2px;
        }
        .db-error{
            display: block;
            float: left;
            width: 100%;
            text-align: left;
            background: #ededed;
        }
        .db-prop{
            margin-bottom: 5px;
            padding: 6px;
        }
        .prop > .db-key{

        }
        .prop > .db-value{
            margin-top: 2px;
            background: #fcfcfc;
            padding: 4px;
        }
    )";
    std::stringstream stream;
    stream << "<html>"
           <<   "<head>"
           <<       "<title>" << (int) _status << " Error " << "</title>"
           <<       "<style>" << css << "</style>"
           <<   "</head>"
           <<   "<body>"
           <<       "<div class='wrapper'>"
           <<           "<div class='status'>" << (int) _status << " " << _status << "</div>"
           <<           "<div class='heading'>" << _heading << "</div>"
           <<           "<div class='main'>";
    for(const block& blk: _blocks)
    stream <<               blk.html();
    stream <<           "</div>"
           <<       "</div>"
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


void udho::exceptions::http_error::decorate(udho::exceptions::visual::page& p) const {}

const char* udho::exceptions::http_error::what() const noexcept{
    return _err_str.c_str();
}

udho::exceptions::visual::page udho::exceptions::http_error::page(const std::string& target) const{
    std::string heading = (boost::format("%1% Error while accessing <span class='resource-path'>%2%</span>") % _status % target).str();
    if(!_message.empty()){
        heading += (boost::format("<div class='error-message'>%1%</div>") % _message).str();
    }
    return udho::exceptions::visual::page(_status, heading);
}

boost::beast::http::status udho::exceptions::http_error::result() const{
    return _status;
}

udho::exceptions::http_error udho::exceptions::http_redirection(const std::string& location, boost::beast::http::status status){
    udho::exceptions::http_error error(status);
    error.redirect(location);
    return error;
}

