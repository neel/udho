#include <set>
#include <map>
#include <stack>
#include <udho/scope.h>
#include <udho/access.h>
#include <udho/parser.h>
#include <udho/configuration.h>

int main(){
    udho::config<udho::configs::server> conf;
    
    conf[udho::configs::server::document_root] = "/path/to/document/root";
    conf[udho::configs::server::template_root] = "/path/to/template/root";
    
    std::string value = conf[udho::configs::server::document_root];
    
    return 0;
}

