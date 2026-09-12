// SPDX-License-Identifier: BSD-3-Clause
#include "activities.h"
#include "db/pg/models.h"

#include <iostream>

namespace models = pgx::setup::db::pg::models;
namespace pg     = udho::db::pg;

void pgx::setup::activities::create_tables(boost::asio::io_context& io, pg::connection::pool& pool, completion complete) {
    using create_projects = models::projects::create;
    using create_tasks    = models::tasks::create;

    auto pipeline = pg::start<create_projects, create_tasks>::with(io, pool);
    auto projects = pg::after(pipeline).perform<create_projects>(pipeline).if_failed([](const pg::failure& failure){
        std::cout << "Error: " << failure.reason << std::endl << failure.sql << std::endl;
        return false;
    });
    auto tasks    = pg::after(projects).perform<create_tasks>(pipeline).if_failed([](const pg::failure& failure){
        std::cout << "Error: " << failure.reason << std::endl << failure.sql << std::endl;
        return false;
    });

    pg::after(tasks).finish(pipeline, [complete = std::move(complete)](const pg::data<create_projects, create_tasks>& data) {
        if(data.failed<create_projects>()) {
            const auto& failure = data.failure<create_projects>();
            std::cout << "Could not create projects: " << failure.reason << std::endl << failure.sql << std::endl;
            complete(false);
            return;
        }
        if(data.failed<create_tasks>()) {
            const auto& failure = data.failure<create_tasks>();
            std::cout << "Could not create tasks: " << failure.reason << std::endl << failure.sql << std::endl;
            complete(false);
            return;
        }
        complete(true);
    });

    pipeline();
}
