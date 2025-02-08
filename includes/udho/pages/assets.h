#ifndef UDHO_PAGES_ASSETS_H
#define UDHO_PAGES_ASSETS_H

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>

namespace udho{
namespace pages{
namespace system{
namespace assets{

constexpr static char css_system[] = R"ASSET(
.central{
    margin-left: 20%;
    margin-right: 20%;
    width: auto;
    position: relative;
}
.central > .header{
    border-bottom: 1px solid #b4b4b4;
}
.central > .footer{
    border-top: 1px solid #b4b4b4;
}

)ASSET";

constexpr static char css_listing[] = R"ASSET(
/* Deepseek R1 generated CSS */

/* Container for responsive overflow */
.udho-directory-container {
    overflow-x: auto;
    border-radius: 8px;
    box-shadow: 0 1px 3px rgba(0,0,0,0.12);
}

/* Modern table styling */
table.udho-directory-listing {
    width: 100%;
    border-collapse: collapse;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen-Sans, Ubuntu, Cantarell, sans-serif;
    font-size: 0.9rem;
    background: white;
    min-width: 600px;
}

/* Header styling */
table.udho-directory-listing thead th {
    font-weight: 500;
    color: #2d3748;
    text-align: left;
    padding: 16px 24px;
    background-color: #f7fafc;
    border-bottom: 2px solid #e2e8f0;
}

/* Table cells */
table.udho-directory-listing td {
    padding: 12px 24px;
    color: #4a5568;
    border-bottom: 1px solid #edf2f7;
}

/* Row hover effect */
table.udho-directory-listing tbody tr {
    transition: background-color 0.2s ease;
}

table.udho-directory-listing tbody tr:hover {
    background-color: #f8fafc;
    cursor: pointer;
}

/* Icon styling */
.udho-icon-file,
.udho-icon-directory {
    font-family: "Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji", sans-serif;
    margin-right: 12px;
    transition: transform 0.2s ease;
}

/* Add these rules */
.udho-icon-file::before,
.udho-icon-directory::before {
    content: "";
    display: inline-block;
    font-family: "Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji", sans-serif;
    width: 1.2em;
    height: 1.2em;
    vertical-align: middle;
}

.udho-icon-file::before {
    content: "\1F4C4"; /* Page Facing Up */
}

.udho-icon-directory::before {
    content: "\1F4C1"; /* File Folder */
}


/* File name link styling */
table.udho-directory-listing a {
    color: #2d3748;
    text-decoration: none;
    transition: color 0.2s ease;
    display: flex;
    align-items: center;
}

table.udho-directory-listing a:hover {
    color: #4299e1;
}

/* Extension badge styling */
table.udho-directory-listing td:nth-child(2) {
    font-size: 0.8em;
    color: #718096;
    font-weight: 500;
}

/* Size formatting */
table.udho-directory-listing td:nth-child(5) {
    font-family: monospace;
    font-size: 0.9em;
}

/* Dark mode support */
@media (prefers-color-scheme: dark) {
    .udho-directory-container {
        box-shadow: 0 1px 3px rgba(0,0,0,0.24);
    }

    table.udho-directory-listing {
        background: #1a202c;
    }

    table.udho-directory-listing thead th {
        background-color: #2d3748;
        color: #cbd5e0;
        border-bottom-color: #4a5568;
    }

    table.udho-directory-listing td {
        color: #cbd5e0;
        border-bottom-color: #2d3748;
    }

    table.udho-directory-listing tbody tr:hover {
        background-color: #2d3748;
    }

    table.udho-directory-listing a {
        color: #cbd5e0;
    }

    table.udho-directory-listing a:hover {
        color: #63b3ed;
    }
}
)ASSET";

template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    store["udho"]   << udho::view::resources::asset::css("system.css",  std::begin(css_system),  std::end(css_system))
                    << udho::view::resources::asset::css("listing.css", std::begin(css_listing), std::end(css_listing));
}

}
}
}
}

#endif // UDHO_PAGES_ASSETS_H
