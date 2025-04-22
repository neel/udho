#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>
#include <udho/net/context.h>
#include <udho/url/router.h>

#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/net/artifacts.h>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

static size_t curl_writef(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct http_results{
    long code;
    std::string   body;
    std::map<std::string, std::string> headers;
};

http_results curl_fetch(CURL* curl, const std::string method, const std::string& url){
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
        struct curl_slist *headers = NULL;
        std::string response_headers;
        std::string response_body;
        std::map<std::string, std::string> headers_map;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curl_writef);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,  curl_writef);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA,      &response_headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_body);
        res = curl_easy_perform(curl);
        long response_code = 0;
        if(res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            std::vector<std::string> header_lines;
            boost::algorithm::split(header_lines, response_headers, boost::is_any_of("\r\n"));
            for(const std::string& line: header_lines){
                std::vector<std::string> header_parts;
                boost::algorithm::split(header_parts, line, boost::is_any_of(":"));
                if(header_parts.size() >= 2){
                    headers_map.insert(std::make_pair(boost::algorithm::trim_copy(header_parts[0]), boost::algorithm::trim_copy(header_parts[1])));
                }
            }
        }
        return http_results{response_code, response_body, headers_map};
}


TEST_CASE("Accessing assets through router via HTTP requests", "[router][asset]") {
    static char buffer_js [] = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static unsigned char buffer_img[] = {
        0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x04, 0x00, 0x04, 0x00, 0x80, 0x01,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x04, 0x00, 0x00, 0x02, 0x05, 0x44, 0x7c, 0x67, 0xb8, 0x05,
        0x00, 0x3b
    };

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    udho::pages::system::setup(resources);

    auto previous_size = resources.assets().size();
    resources["primary"] << udho::view::resources::asset::js ("0profile1.js", std::begin(buffer_js),  std::end(buffer_js) );
    resources["primary"] << udho::view::resources::asset::js ("1profile2.js", std::begin(buffer_js1), std::end(buffer_js1));
    resources["primary"] << udho::view::resources::asset::css("2profile.css", std::begin(buffer_css), std::end(buffer_css));
    resources["primary"] << udho::view::resources::asset::img("3profile.gif", std::begin(buffer_img), std::end(buffer_img));// ->mime("image/gif");

    CHECK(previous_size + 4 == resources.assets().size());

    resources.assets().base("assets");

    // TEST Creating a const_store from store should throw exception unless the store is locked.
    REQUIRE_THROWS_AS(udho::view::resources::const_store<udho::view::data::bridges::lua>{resources}, std::exception);

    resources.lock();
    // TEST Adding resources to a locked store should also throw exception
    REQUIRE_THROWS_AS(resources["primary"] << udho::view::resources::asset::js("profile1.js", buffer_js, buffer_js+std::strlen(buffer_js)), std::exception);

    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{resources};

    std::filesystem::path current          = std::filesystem::current_path();
    std::filesystem::path docroot          = current / "docroot";
    std::filesystem::path assets_docroot   = docroot / "assets" / "primary";
    std::filesystem::path alternate        = current / "alt";
    std::filesystem::path assets_alternate = alternate / "assets" / "primary";

    if(std::filesystem::exists(docroot) && std::filesystem::is_directory(docroot)){
        std::filesystem::remove_all(docroot);
    }
    std::filesystem::create_directory(docroot);
    std::filesystem::create_directories(assets_docroot);
    if(std::filesystem::exists(alternate) && std::filesystem::is_directory(alternate)){
        std::filesystem::remove_all(alternate);
    }
    std::filesystem::create_directory(alternate);

    udho::url::explorers::files d{"docroot", docroot};
    udho::url::mime_registry mimes = d.mimes();
    auto router = udho::url::router(
        std::move(d),
        udho::url::explorers::assets{"assets", cstore.assets()},
        udho::url::explorers::files{"alternate", alternate}
    );

    std::map<std::string, std::string> alternate_files = {
        {"c.txt", "c"},
        {"d.txt", "d"}
    };
    for(const auto& f: alternate_files){
        std::filesystem::path path = alternate / f.first;
        if(!std::filesystem::exists(path)){
            std::ofstream stream{path};
            stream << f.second;
        }
    }

    std::cout << router << std::endl;

    boost::asio::io_context service;

    auto server = udho::net::server<http_listener>(service, 9000);
    auto artifacts  = udho::net::artifacts{router, resources};

    server.run(artifacts);

    std::thread thread([&]{
        service.run();
    });

    CHECK(0 == 0);

    CURL* curl;
    curl = curl_easy_init();
    CHECK(curl != 0x0);

    SECTION("Listing Datastructure"){
        std::map<std::string, std::string> docroot_files = {
            {"a.txt", "a"},
            {"b.js", "b"},
            {"c.css", "!C"}
        };
        for(const auto& f: docroot_files){
            std::filesystem::path path = docroot / f.first;
            if(!std::filesystem::exists(path)){
                std::ofstream stream{path};
                stream << f.second;
            }
        }

        std::ofstream stream{docroot / "h.gif", std::ios::binary};
        stream.write(reinterpret_cast<const char*>(buffer_img), sizeof(buffer_img));
        stream.close();

        SECTION("Checking static files") {
            for (const auto& entry_fs : std::filesystem::directory_iterator(docroot)) {
                udho::pages::system::data::entry e(entry_fs, docroot, mimes);

                // Get filename without path
                std::string filename = entry_fs.path().filename().string();

                if (filename == "assets") {
                    // Test directory handling
                    THEN("Directory entry is correct") {
                        CHECK(e.name() == "assets");
                        CHECK(e.is_directory() == true);
                        CHECK(e.mime() == "N/A");
                        CHECK(e.url() == "/assets/");
                        CHECK(e.type() == "Directory");
                        CHECK(e.extension().empty());
                        CHECK(e.size() == "N/A");
                    }
                }
                else {
                    // Test file entries
                    THEN("File entry for " + filename + " is correct") {
                        // Basic properties
                        CHECK(e.name() == filename);
                        CHECK(e.is_directory() == false);
                        CHECK(e.type() == "File");

                        // URL check
                        std::string expected_url = "/" + filename;
                        CHECK(e.url() == expected_url);

                        // Extension check
                        size_t dot_pos = filename.rfind('.');
                        if (dot_pos != std::string::npos) {
                            std::string expected_ext = filename.substr(dot_pos + 1);
                            CHECK(e.extension() == expected_ext);
                        }

                        // Size formatting check
                        if (filename == "h.gif") {
                            // Verify image size formatting
                            std::string size_str = e.size();
                            CAPTURE(size_str);
                            CHECK(size_str.find("B") != std::string::npos);

                            // Convert back to bytes for verification
                            double size_value = std::stod(size_str.substr(0, size_str.find(' ')));
                            std::string unit = size_str.substr(size_str.find(' ') + 1);

                            if (unit == "B") CHECK(sizeof(buffer_img) == static_cast<size_t>(size_value));
                            else if (unit == "KB") CHECK(sizeof(buffer_img) == static_cast<size_t>(size_value * 1024));
                        }
                        else {
                            // Verify text file sizes
                            std::string expected_content = docroot_files.at(filename);
                            std::string size_str = e.size();

                            if (expected_content.size() < 1024) {
                                CHECK(size_str == std::to_string(expected_content.size()) + " B");
                            }
                        }

                        // MIME type validation
                        if (filename == "a.txt") CHECK(e.mime() == "text/plain");
                        else if (filename == "b.js") CHECK(e.mime() == "text/javascript");
                        else if (filename == "c.css") CHECK(e.mime() == "text/css");
                        else if (filename == "h.gif") CHECK(e.mime() == "image/gif");
                    }
                }
            }

            // Verify all expected files were found
            std::set<std::string> expected_files;
            for (const auto& [file, _] : docroot_files) {
                expected_files.insert(file);
            }
            expected_files.insert("h.gif");
            expected_files.insert("assets");

            std::set<std::string> found_files;
            for (const auto& entry_fs : std::filesystem::directory_iterator(docroot)) {
                found_files.insert(entry_fs.path().filename().string());
            }

            CHECK(found_files == expected_files);
        }


    }

    SECTION("Overriding static files") {
        SECTION("Alternate files accessible") {
            {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/c.txt");
                CHECK(results.code == 200);
                CHECK(results.body.size() == 1);
                CHECK(results.body == "c");
                CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
                CHECK(results.headers["Content-Type"] == "text/plain");
            } {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/d.txt");
                CHECK(results.code == 200);
                CHECK(results.body.size() == 1);
                CHECK(results.body == "d");
                CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
                CHECK(results.headers["Content-Type"] == "text/plain");
            } {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/b.txt");
                CHECK(results.code == 404);
            }
        }
        std::map<std::string, std::string> docroot_files = {
            {"a.txt", "a"},
            {"b.txt", "b"},
            {"c.txt", "!C"}
        };
        for(const auto& f: docroot_files){
            std::filesystem::path path = docroot / f.first;
            if(!std::filesystem::exists(path)){
                std::ofstream stream{path};
                stream << f.second;
            }
        }
        SECTION("docroot files accessible") {
            {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/a.txt");
                CHECK(results.code == 200);
                CHECK(results.body.size() == 1);
                CHECK(results.body == "a");
                CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
                CHECK(results.headers["Content-Type"] == "text/plain");
            } {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/d.txt");
                CHECK(results.code == 200);
                CHECK(results.body.size() == 1);
                CHECK(results.body == "d");
                CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
                CHECK(results.headers["Content-Type"] == "text/plain");
            }
        }
        SECTION("Alternate files overridden by docroot") {
            {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/c.txt");
                CHECK(results.code == 200);
                CHECK(results.body.size() == 2);
                CHECK(results.body == "!C");
                CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
                CHECK(results.headers["Content-Type"] == "text/plain");
            }
        }
        SECTION("Non existent files yield 404") {
            {
                http_results results = curl_fetch(curl, "GET", "http://localhost:9000/e.txt");
                CHECK(results.code == 404);
            }
        }
    }

    SECTION("Assets accessible from asset store") {
        SECTION("HTTP Response js0") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/0profile1.js");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_js));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_js)));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "application/javascript");
        }

        SECTION("HTTP Response js1") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/1profile2.js");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_js1));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_js1)));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "application/javascript");
        }

        SECTION("HTTP Response css") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/2profile.css");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_css));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_css)));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "text/css");
        }

        SECTION("HTTP Response img") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/3profile.gif");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_img));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_img), [](const char& l, const unsigned char& r){ return static_cast<unsigned char>(l) == r; }));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "image/gif");
        }
    }

    SECTION("Overriding Assets"){
        std::string altered_content = ".classname{color: red}";
        std::filesystem::path profile_css_overriding_path = assets_docroot / "2profile.css";
        {
            std::ofstream stream{profile_css_overriding_path};
            CHECK(stream.is_open());
            stream << altered_content;
        }

        SECTION("Overriding asset store") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/2profile.css");
            CHECK(results.code == 200);
            CHECK(results.body.size() == altered_content.size());
            CHECK(results.body == altered_content);
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "text/css");
        }

        std::filesystem::remove(profile_css_overriding_path);
        SECTION("Deleting docroot asset falls back to asset store") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/2profile.css");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_css));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_css)));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "text/css");
        }

        std::filesystem::create_directories(assets_alternate);
        std::filesystem::path profile_css_overriding_path_alt = assets_alternate / "2profile.css";
        {
            std::ofstream stream{profile_css_overriding_path_alt};
            CHECK(stream.is_open());
            stream << altered_content;
        }

        SECTION("Alternate does not override asset store") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/2profile.css");
            CHECK(results.code == 200);
            CHECK(results.body.size() == sizeof(buffer_css));
            CHECK(std::equal(results.body.begin(), results.body.end(), std::begin(buffer_css)));
            CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
            CHECK(results.headers["Content-Type"] == "text/css");
        }
    }

    SECTION("Listing"){
        std::map<std::string, std::string> docroot_files = {
            {"a.txt", "a"},
            {"b.txt", "b"},
            {"c.txt", "!C"}
        };
        std::string altered_content = ".classname{color: red}";
        std::filesystem::path profile_css_overriding_path = assets_docroot / "2profile.css";
        {
            std::ofstream stream{profile_css_overriding_path};
            CHECK(stream.is_open());
            stream << altered_content;
        }
        for(const auto& f: docroot_files){
            std::filesystem::path path = docroot / f.first;
            if(!std::filesystem::exists(path)){
                std::ofstream stream{path};
                stream << f.second;
            }
        }

        auto extract_links = [](const std::string& body, std::multimap<std::string, std::string>& links){
            std::regex pattern("<a\\s+class=\"[^\"]*udho-listing-item-([^\"]+)[^\"]*\"\\s+href=\"([^\"]+)\">");

            auto begin = std::sregex_iterator(body.begin(), body.end(), pattern);
            auto end = std::sregex_iterator();

            for (std::sregex_iterator i = begin; i != end; ++i) {
                std::smatch match = *i;
                std::string key = match[1].str();
                std::string value = match[2].str();

                bool exists = false;
                auto range = links.equal_range(key);
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second == value) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) {
                    links.insert(std::make_pair(key, value));
                }
            }
        };

        auto validate_observation = [](const std::multimap<std::string, std::string>& captured_links, const std::string& label, const std::set<std::string>& expected_items_set){
            std::set<std::string> observed_items_set;
            auto items = captured_links.equal_range(label);
            for (auto it = items.first; it != items.second; ++it) {
                std::string item = it->second;
                if (!item.empty() && item[0] == '/') {
                    item.erase(0, 1);
                }
                observed_items_set.insert(item);
            }

            std::vector<std::string> difference;
            std::set_symmetric_difference(
                observed_items_set.begin(), observed_items_set.end(),
                expected_items_set.begin(), expected_items_set.end(),
                std::back_inserter(difference)
                );

            INFO("Checking listing for explorer " << label);
            CAPTURE(observed_items_set, expected_items_set);
            CHECK(difference.empty());
        };

        SECTION("static files listed under docroot and alternate") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot, expected_items_set_alternate;
            for (const auto& [filename, _] : docroot_files) {
                expected_items_set_docroot.insert(filename);
            }
            expected_items_set_docroot.insert("assets/");
            for (const auto& [filename, _] : alternate_files) {
                expected_items_set_alternate.insert(filename);
            }

            validate_observation(captured_links, "docroot",   expected_items_set_docroot);
            validate_observation(captured_links, "alternate", expected_items_set_alternate);
        }

        SECTION("static files listed under docroot and alternate without / in the end") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot, expected_items_set_alternate;
            for (const auto& [filename, _] : docroot_files) {
                expected_items_set_docroot.insert(filename);
            }
            expected_items_set_docroot.insert("assets/");
            for (const auto& [filename, _] : alternate_files) {
                expected_items_set_alternate.insert(filename);
            }

            validate_observation(captured_links, "docroot",   expected_items_set_docroot);
            validate_observation(captured_links, "alternate", expected_items_set_alternate);
        }

        SECTION("assets listed under docroot and asset store") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot = { "assets/primary/" };
            std::set<std::string> expected_items_set_assets = { "assets/primary/", "assets/udho/" };
            validate_observation(captured_links, "docroot", expected_items_set_docroot);
            validate_observation(captured_links, "assets",  expected_items_set_assets);
        }

        SECTION("assets listed under docroot and asset store with / in the end") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot = { "assets/primary/" };
            std::set<std::string> expected_items_set_assets = { "assets/primary/", "assets/udho/" };
            validate_observation(captured_links, "docroot", expected_items_set_docroot);
            validate_observation(captured_links, "assets",  expected_items_set_assets);
        }

        SECTION("assets listed under docroot and asset store prefix") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot = { "assets/primary/2profile.css" };
            std::set<std::string> expected_items_set_assets = { "assets/primary/0profile1.js", "assets/primary/1profile2.js", "assets/primary/2profile.css", "assets/primary/3profile.gif" };
            validate_observation(captured_links, "docroot", expected_items_set_docroot);
            validate_observation(captured_links, "assets",  expected_items_set_assets);
        }

        SECTION("assets listed under docroot and asset store prefix with / in the end") {
            http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/");
            CHECK(results.code == 200);
            std::string body = results.body;
            std::multimap<std::string, std::string> captured_links;
            extract_links(body, captured_links);
            std::set<std::string> expected_items_set_docroot = { "assets/primary/2profile.css" };
            std::set<std::string> expected_items_set_assets = { "assets/primary/0profile1.js", "assets/primary/1profile2.js", "assets/primary/2profile.css", "assets/primary/3profile.gif" };
            validate_observation(captured_links, "docroot", expected_items_set_docroot);
            validate_observation(captured_links, "assets",  expected_items_set_assets);
        }

    }

    curl_easy_cleanup(curl);

    server.stop();

    thread.join();
}
