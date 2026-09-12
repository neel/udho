// SPDX-License-Identifier: BSD-3-Clause
#include "activities.h"

#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <udho/www/components/pg.h>

int main() {
    boost::asio::io_context io;
    const char* configured = std::getenv("UDHO_PGX_DATABASE_URL");
    const std::string connection_string = configured ? configured : "dbname=udho_pgx user=postgres";
    udho::www::components::db::pg<> database(connection_string);
    bool succeeded = false;

    pgx::setup::activities::create_tables(io, database.pool(), [&succeeded](bool result) {
        succeeded = result;
        if(result) std::cout << "Created pgx tables." << std::endl;
    });

    io.run();
    return succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
