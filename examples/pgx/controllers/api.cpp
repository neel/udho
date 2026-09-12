// SPDX-License-Identifier: BSD-3-Clause
#include "api.h"
#include "../db/pg/models.h"

#include <nlohmann/json.hpp>
#include <udho/db/pg/io/json.h>

namespace model  = pgx::db::pg::models;
namespace schema = pgx::db::pg::schema;
namespace pg     = udho::db::pg;

pgx::controllers::api::api(boost::asio::io_context& io, database_type& database): _io(io), _database(database) {}

void pgx::controllers::api::projects(context context) {
    using query = model::projects::all;
    auto pipeline = pg::start<query>::with(_io, _database.pool());
    auto projects = pg::after(pipeline).perform<query>(pipeline);

    pg::after(projects).finish(pipeline, [context](const pg::data<query>& data) mutable {
        context.ostream().set(boost::beast::http::field::content_type, "application/json");
        if(data.failed<query>()) {
            context.ostream().status(boost::beast::http::status::internal_server_error);
            context << nlohmann::json{{"error", "project query failed"}}.dump(2);
        } else {
            context << nlohmann::json{{"projects", data.success<query>()}}.dump(2);
        }
        context.finish();
    });

    pipeline();
}

void pgx::controllers::api::project(context context, int id) {
    using project_query     = model::projects::by_id;
    using tasks_query       = model::tasks::by_project;
    using incomplete_query  = model::tasks::incomplete_by_project;

    auto pipeline   = pg::start<project_query, tasks_query, incomplete_query>::with(_io, _database.pool());
    auto project    = pg::after(pipeline).perform<project_query>(pipeline);
    auto tasks      = pg::after(project).perform<tasks_query>(pipeline);
    auto incomplete = pg::after(project).perform<incomplete_query>(pipeline);

    project[schema::projects::id::val]          = id;
    tasks[schema::tasks::project::val]          = id;
    incomplete[schema::tasks::project::val]     = id;
    incomplete[schema::tasks::completed::val]   = false;

    pg::after(tasks, incomplete).finish(pipeline, [context](const pg::data<project_query, tasks_query, incomplete_query>& data) mutable {
            context.ostream().set(boost::beast::http::field::content_type, "application/json");
            if(data.failed<project_query>() || data.failed<tasks_query>() || data.failed<incomplete_query>()) {
                context.ostream().status(boost::beast::http::status::internal_server_error);
                context << nlohmann::json{{"error", "project pipeline failed"}}.dump(2);
            } else if(data.success<project_query>().empty()) {
                context.ostream().status(boost::beast::http::status::not_found);
                context << nlohmann::json{{"error", "project not found"}}.dump(2);
            } else {
                context << nlohmann::json{
                    {"project", data.success<project_query>()},
                    {"tasks", data.success<tasks_query>()},
                    {"incomplete_tasks", data.success<incomplete_query>()},
                    {"counts", {
                                    {"all", data.success<tasks_query>().count()},
                                    {"incomplete", data.success<incomplete_query>().count()}
                               }
                    }
                }.dump(2);
            }
            context.finish();
        })->force(true);

    pipeline();
}
