// SPDX-License-Identifier: BSD-3-Clause
#include "ui.h"
#include "../db/pg/models.h"
#include "../view/data/project.h"

#include <udho/net/protocols/form_data.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/www/www.h>

namespace model         = pgx::db::pg::models;
namespace schema        = pgx::db::pg::schema;
namespace pg            = udho::db::pg;
namespace layout        = udho::view::tmpl::layout;
namespace placeholders  = udho::view::tmpl::layout::placeholders;

pgx::controllers::ui::ui(boost::asio::io_context& io, database_type& database): _io(io), _database(database) {}

void pgx::controllers::ui::home(context context) {
    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    layout::standard_layout<decltype(context)> page(context);
    page.preamble().title("Udho PostgreSQL example").doclang("en");
    page.css().add("pgx", "app.css");

    page[placeholders::header] = "<h1>Udho PostgreSQL UI</h1>";
    page[placeholders::central] = R"(
      <p><a href="/projects/new">Create your first project</a></p>
      <p>The same project is rendered in two different ways:</p>
      <ul>
        <li><a href="/projects/1.html">server-rendered Lua view</a></li>
        <li><a href="/projects/1">static fragment populated from the JSON API</a></li>
      </ul>
      <p>The API is also available at <a href="/api/projects">/api/projects</a>.</p>)";
    page[placeholders::footer] = "<small>Database work is asynchronous and pipelined.</small>";

    page();
}

void pgx::controllers::ui::new_project(context context) {
    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    layout::standard_layout<decltype(context)> page(context);
    page.preamble().title("Create a project").doclang("en");
    page.css().add("pgx", "app.css");

    page[placeholders::header] = "<h1>Create a project</h1>";
    page[placeholders::central] = R"(
      <form method="post" action="/projects">
        <p><label>Title <input name="title" required></label></p>
        <p><label>Description<br><textarea name="description" rows="4" required></textarea></label></p>
        <button type="submit">Create project</button>
      </form>)";
    page[placeholders::footer] = "<small>The form is stored through a PostgreSQL insert activity.</small>";

    page();
}

void pgx::controllers::ui::create_project(context context) {
    using query = model::projects::create;
    const udho::net::protocols::detail::form_view form(context.portal().body().form());

    if(form.count("title") == 0 || form.count("description") == 0 || form.field("title").string().empty() || form.field("description").string().empty()) {
        context.ostream().status(boost::beast::http::status::bad_request);
        context << "A title and description are required.";
        context.finish();
        return;
    }

    auto pipeline = pg::start<query>::with(_io, _database.pool());
    auto project = pg::after(pipeline).perform<query>(pipeline);
    project[schema::projects::title::val] = pg::oz::varchar(form.field("title").string());
    project[schema::projects::description::val] = form.field("description").string();

    pg::after(project).finish(pipeline, [context](const pg::data<query>& data) mutable {
        if(data.failed<query>()) {
            context.ostream().status(boost::beast::http::status::internal_server_error);
            context << "Could not create the project.";
        } else {
            const auto& row = *data.success<query>();
            const auto location = "/projects/" + std::to_string(row[schema::projects::id::val].value()) + ".html";
            context.ostream().status(boost::beast::http::status::see_other);
            context.ostream().set(boost::beast::http::field::location, location);
        }
        context.finish();
    })->force(true);

    pipeline();
}

void pgx::controllers::ui::create_task(context context, int project_id) {
    using query = model::tasks::create;
    const udho::net::protocols::detail::form_view form(context.portal().body().form());

    if(form.count("title") == 0 || form.field("title").string().empty()) {
        context.ostream().status(boost::beast::http::status::bad_request);
        context << "A task title is required.";
        context.finish();
        return;
    }

    auto pipeline = pg::start<query>::with(_io, _database.pool());
    auto task = pg::after(pipeline).perform<query>(pipeline);
    task[schema::tasks::project::val] = project_id;
    task[schema::tasks::title::val] = pg::oz::varchar(form.field("title").string());
    task[schema::tasks::completed::val] = false;

    pg::after(task).finish(pipeline, [context, project_id](const pg::data<query>& data) mutable {
        if(data.failed<query>()) {
            context.ostream().status(boost::beast::http::status::internal_server_error);
            context << "Could not create the task.";
        } else {
            context.ostream().status(boost::beast::http::status::see_other);
            context.ostream().set(boost::beast::http::field::location, "/projects/" + std::to_string(project_id) + ".html");
        }
        context.finish();
    })->force(true);

    pipeline();
}

void pgx::controllers::ui::project(context context, int id) {
    std::cout << "resource project/" << id << std::endl;
    if(context.portal().query().extension() == "html")
        project_html(context, id);
    else
        project_ajax(context, id);
}

void pgx::controllers::ui::project_ajax(context context, int id) {
    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    layout::standard_layout<decltype(context)> page(context);
    page.preamble().title("Project loaded with Ajax").doclang("en");

    page.css().add("pgx", "app.css");
    page.js().add("pgx", "app.js");

    page[placeholders::header] = "<h1>Client-rendered project</h1>";
    page[placeholders::central] = "<main id=\"project-root\" data-project-id=\"" + std::to_string(id) + "\"></main>";
    page[placeholders::footer] = "<small>The shell and fragment are static; data comes from /api.</small>";

    page();
}

void pgx::controllers::ui::project_html(context context, int id) {
    using project_query = model::projects::by_id;
    using tasks_query   = model::tasks::by_project;

    auto pipeline   = pg::start<project_query, tasks_query>::with(_io, _database.pool());
    auto project    = pg::after(pipeline).perform<project_query>(pipeline);
    auto tasks      = pg::after(project).perform<tasks_query>(pipeline);

    project[schema::projects::id::val] = id;
    tasks[schema::tasks::project::val] = id;

    pg::after(tasks).finish(pipeline, [context](const pg::data<project_query, tasks_query>& data) mutable {
        if(data.failed<project_query>() || data.failed<tasks_query>()) {
            context.ostream().status(boost::beast::http::status::internal_server_error);
            context << "Database query failed!";
            context.finish();
            return;
        }
        const auto& projects = data.success<project_query>();
        if(projects.empty()) {
            context.ostream().status(boost::beast::http::status::not_found);
            context << "Project not found!";
            context.finish();
            return;
        }
        const auto& row = *projects;
        auto value = pgx::view::data::make_project(static_cast<int>(row[schema::projects::id::val].value()), row[schema::projects::title::val].value(), row[schema::projects::description::val].value());
        for(const auto& task: data.success<tasks_query>()) {
            pgx::view::data::add_task(value, static_cast<int>(task[schema::tasks::id::val].value()), task[schema::tasks::title::val].value(), task[schema::tasks::completed::val].value());
        }
        context.ostream().set(boost::beast::http::field::content_type, "text/html");
        layout::standard_layout<decltype(context)> page(context);
        page.preamble().title(value.title).doclang("en");
        page.css().add("pgx", "app.css");
        page[placeholders::header] = "<h1>Server-rendered project</h1>";
        page.properties(placeholders::central).view("lua://pgx/project.view");
        page[placeholders::central] = value;
        page[placeholders::footer] = "<small>PostgreSQL data rendered by a Lua view.</small>";
        page();
    })->force(true);

    pipeline();
}
