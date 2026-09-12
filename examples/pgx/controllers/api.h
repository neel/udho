// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_CONTROLLERS_API_H
#define UDHO_EXAMPLES_PGX_CONTROLLERS_API_H

#include <boost/asio/io_context.hpp>
#include <udho/url/url.h>
#include <udho/www/components/pg.h>
#include <udho/www/context.h>

namespace pgx::controllers {

class api {
public:
    using database_type = udho::www::components::db::pg<>;
    using context       = udho::www::context<>;

    api(boost::asio::io_context& io, database_type& database);
    auto routes() {
        using namespace udho::hazo::string::literals;
        return udho::url::slot("projects"_h, &api::projects, this)   << udho::url::fixed(udho::url::verb::get, "/projects", "/projects")
             | udho::url::slot("api-project"_h, &api::project, this) << udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}");
    }

    void projects(context context);
    void project(context context, int id);

private:
    boost::asio::io_context& _io;
    database_type& _database;
};

}
#endif
