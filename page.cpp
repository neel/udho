#include "udho/page.h"
#include <boost/format.hpp>

udho::exceptions::http_error::http_error(boost::beast::http::status status, const std::string& message): _status(status), _message(message){

}

const char* udho::exceptions::http_error::what() const noexcept{
    return (boost::format("%1% Error") % _status).str().c_str();
}

std::string udho::exceptions::http_error::page(const std::string& target, std::string content) const{
    std::string message = (boost::format("%1% Error while accessing <span class='resource-path'>%2%</span>") % _status % target).str();
    if(!_message.empty()){
        message += (boost::format("<div class='error-message'>%1%</div>") % _message).str();
    }
    std::string html    = R"page(
    <html>
        <head>
            <title>%1% Error</title>
            <style>
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
            </style>
        </head>
        <body>
            <div class="wrapper">
                <div class="block">
                    <div class="header">%1% %2%</div>
                    %3%
                </div>
                <div class="main">
                    %4%
                </div>
            </div>
        </body>
    </html>
    )page";
    return (boost::format(html) % (int)_status % _status % message % content).str();
}

boost::beast::http::status udho::exceptions::http_error::result() const{
    return _status;
}



