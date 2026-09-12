// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_VIEW_DATA_PROJECT_H
#define UDHO_EXAMPLES_PGX_VIEW_DATA_PROJECT_H

#include <string>
#include <vector>
#include <udho/view/data.h>
#include <udho/view/meta.h>

namespace pgx::view::data {

struct task {
    int id{};
    std::string title;
    bool completed{};

    friend auto metatype(udho::view::data::type<task>) {
        using namespace udho::view::data;
        return assoc("pgx_task"),
            cvar("id", &task::id),
            cvar("title", &task::title),
            cvar("completed", &task::completed);
    }
};

struct project {
    int id{};
    std::string title;
    std::string description;
    std::vector<task> tasks;

    friend auto metatype(udho::view::data::type<project>) {
        using namespace udho::view::data;
        return assoc("pgx_project"),
            cvar("id", &project::id),
            cvar("title", &project::title),
            cvar("description", &project::description),
            cvar("tasks", &project::tasks);
    }
};

project make_project(int id, std::string title, std::string description);
void add_task(project& value, int id, std::string title, bool completed);

}
#endif
