// SPDX-FileCopyrightText: 2024 <copyright holder> <email>
// SPDX-License-Identifier: BSD-3-Clause

#include "user.h"

void simple::apps::user::home(udho::net::context context){
    context << __PRETTY_FUNCTION__;
    context.finish();
}

void simple::apps::user::profile(udho::net::context context, std::size_t id){
    context << __PRETTY_FUNCTION__;
    context.finish();
}

