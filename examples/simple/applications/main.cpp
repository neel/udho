// SPDX-FileCopyrightText: 2024 <copyright holder> <email>
// SPDX-License-Identifier: BSD-3-Clause

#include "main.h"

void simple::apps::main::home(udho::net::stream context){
    context << __PRETTY_FUNCTION__;
    context.finish();
}

void simple::apps::main::contact(udho::net::stream context){
    context << __PRETTY_FUNCTION__;
    context.finish();
}

