// SPDX-License-Identifier: BSD-3-Clause

#include "urls.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/asio/io_context.hpp>
#include <udho/cli/server.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <udho/view/resources/blobs/setup.h>
#include <udho/www/www.h>

using framework_type = udho::www::framework<udho::www::https::stateless::lua>;

int main(int argc, char** argv) {
    std::string tls_certificate;
    std::string tls_private_key;

    udho::cli::control control;
    control.option("tls-certificate", tls_certificate, std::string{"certificates/localhost-cert.pem"}, "PEM certificate chain");
    control.option("tls-private-key", tls_private_key, std::string{"certificates/localhost-key.pem"}, "PEM private key");
    control.parse(argc, argv);
    if(control.admin()) return control();

    boost::asio::io_context io;

    udho::view::data::bridges::lua lua;
    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};

    udho::pages::system::setup(store);
    store.assets().base("assets");
    udho::view::resources::blobs::setup(store, "hello");

    udho::www::https::transport transport(control.ipv4());
    boost::system::error_code error = transport.credentials(tls_certificate, tls_private_key);
    if(error) {
        std::cerr << "Unable to load HTTPS credentials from " << tls_certificate << " and " << tls_private_key << ": " << error.message() << std::endl;
        return EXIT_FAILURE;
    }

    auto server = control.prepare<framework_type>(transport, store, hello::urls());
    server.run(io);

    return EXIT_SUCCESS;
}
