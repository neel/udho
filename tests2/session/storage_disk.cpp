#include <catch2/catch_test_macros.hpp>

#include <udho/session/session.h>
#include <udho/session/storage/disk.h>
#include <udho/utils/filesystem.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

TEST_CASE("session storage disk", "[session][storage][disk]") {
    REQUIRE(0 == 0);

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    udho::session::storage::disk disk{root};

    boost::uuids::random_generator uuid_generator;

    SECTION("record storage and retrieval") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("name", "Neel Basu");
        record_data.set("weight", 48);

        CHECK_NOTHROW(disk.create(record_data));

        std::string session_file_name = boost::uuids::to_string(record_data.sessid()) + ".udho.session";
        udho::utils::filesystem::path expected_path = root / session_file_name;
        CHECK(udho::utils::filesystem::exists(expected_path));

        udho::session::record_data record{record_data.sessid()};
        CHECK_NOTHROW(disk.fetch(record));

        CHECK(record.size() == record_data.size());
        CHECK(record.get<std::string>("name") == record_data.get<std::string>("name"));
        CHECK(record.get<int>("weight") == record_data.get<int>("weight"));

        CHECK("Neel Basu" == record_data.get<std::string>("name"));
        CHECK(48 == record_data.get<int>("weight"));
    }

}
