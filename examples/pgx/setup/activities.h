// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_SETUP_ACTIVITIES_H
#define UDHO_EXAMPLES_PGX_SETUP_ACTIVITIES_H

#include <boost/asio/io_context.hpp>
#include <functional>
#include <udho/db/pg.h>

namespace pgx::setup::activities {

using completion = std::function<void(bool)>;

void create_tables(boost::asio::io_context& io, udho::db::pg::connection::pool& pool, completion complete);

}
#endif
