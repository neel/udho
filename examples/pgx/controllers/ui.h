// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_CONTROLLERS_UI_H
#define UDHO_EXAMPLES_PGX_CONTROLLERS_UI_H

#include <boost/asio/io_context.hpp>
#include <udho/url/url.h>
#include <udho/view/bridges/lua.h>
#include <udho/www/components/pg.h>
#include <udho/www/components/navigator.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/resources.h>
#include <udho/www/context.h>

namespace pgx::controllers {

class ui {
public:
    using lua_bridge    = udho::view::data::bridges::lua;
    using database_type = udho::www::components::db::pg<>;
    using protocol_type = udho::www::components::protocols::http<udho::net::types::socket>;
    using context       = udho::www::context<protocol_type, udho::www::components::navigators::pretty, udho::www::components::resources<lua_bridge>>;

    ui(boost::asio::io_context& io, database_type& database);
    auto routes() {
        using namespace udho::hazo::string::literals;
        return udho::url::slot("home"_h, &ui::home, this)                << udho::url::home(udho::url::verb::get)
             | udho::url::slot("new-project"_h, &ui::new_project, this) << udho::url::fixed(udho::url::verb::get, "/projects/new", "/projects/new")
             | udho::url::slot("create-project"_h, &ui::create_project, this) << udho::url::fixed(udho::url::verb::post, "/projects", "/projects")
             | udho::url::slot("create-task"_h, &ui::create_task, this) << udho::url::regx(udho::url::verb::post, "/projects/(\\d+)/tasks", "/projects/{}/tasks")
             | udho::url::slot("project"_h, &ui::project, this)         << udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}");
    }

    void home(context context);
    void project(context context, int id);
    void new_project(context context);
    void create_project(context context);
    void create_task(context context, int project_id);

private:
    void project_html(context context, int id);
    void project_ajax(context context, int id);

    boost::asio::io_context& _io;
    database_type& _database;
};

}
#endif
