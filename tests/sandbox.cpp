#include <set>
#include <map>
#include <stack>
#include <udho/scope.h>
#include <udho/access.h>
#include <udho/parser.h>
#include <udho/configuration.h>

int main(){
    udho::config<udho::configs::server> conf;
    
    conf[udho::configs::server::port] =  9198;
    conf[udho::configs::server::document_root] = "/path/to/document/root";
    
    std::string value = conf[udho::configs::server::document_root];
    
//     std::cout << conf[udho::configs::server::document_root] << std::endl;
    
    return 0;
}

