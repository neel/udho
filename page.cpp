#include "udho/page.h"
#include <boost/format.hpp>

void udho::internal::html_dump_module_info(const std::vector<udho::module_info>& infos, std::string& buffer){
    static int indent = 0;
    for(auto i = infos.rbegin(); i != infos.rend(); ++i){
        auto info = *i;
        
        if(info._compositor == "UNSPECIFIED") continue;
        for(int j = 0; j < indent; ++j) buffer += "\t";
        
        std::string overload_template = R"(
            <div class='overload'>
                <div class='column method'>%1%</div>
                <div class='column pattern'>%2%</div>
                <div class='column fptr'>%3%</div>
                <div class='column compositor'>(%4%)</div>
            </div>
        )";
        std::string application_template = R"(
            <div class='application'>
                <div class='heading'></div>
                <div class='overloads'>
                    %1%
                </div>
            </div>
        )";
        
        
        if(info._children.size() > 0){
            ++indent;
            std::string module_info_buffer;
            module_info_buffer += (boost::format(overload_template) % info._method % info._pattern % info._fptr % info._compositor).str();
            html_dump_module_info(info._children, module_info_buffer);
            buffer += (boost::format(application_template) % module_info_buffer).str();
            --indent;
        }else{
            buffer += (boost::format(overload_template) % info._method % info._pattern % info._fptr % info._compositor).str();
        }
    }
}

udho::exceptions::http_error::http_error(boost::beast::http::status status, const std::string& resource): _status(status), _resource(resource){

}

const char* udho::exceptions::http_error::what() const noexcept{
    return (boost::format("%1% Error while accessing %2%") % _status % _resource).str().c_str();
}

std::string udho::exceptions::http_error::page(std::string content) const{
    std::string message = (boost::format("%1% Error while accessing <span class='resource-path'>%2%</span>") % _status % _resource).str();
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
                .overloads{
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
                }
                .overload{
                    display: table-row;
                }
                .column{
                    display: table-cell;
                    padding-right: 22px;
                    text-overflow: ellipsis;
                    white-space: nowrap;
                }
                .method{

                }
                .pattern{

                }
                .fptr{

                }
                .compositor{

                }
                .application{
                    display: table-row;
                    width: 100%%;
                    float: left;
                    position: relative;
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



