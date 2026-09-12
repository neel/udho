// SPDX-License-Identifier: BSD-3-Clause
#include "project.h"
#include <utility>

pgx::view::data::project pgx::view::data::make_project(
    int id, std::string title, std::string description) {
    return {id, std::move(title), std::move(description), {}};
}

void pgx::view::data::add_task(
    project& value, int id, std::string title, bool completed) {
    value.tasks.push_back({id, std::move(title), completed});
}
