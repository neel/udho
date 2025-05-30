#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/session/session.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/utils/filesystem.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

TEST_CASE("session storage fs", "[session][storage][fs]") {
    REQUIRE(0 == 0);

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    udho::session::storage::fs disk{root};

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

    SECTION("save-only roundtrip") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("a","1");
        disk.save(record_data);

        record_data.remove("a");
        record_data.set("b","2");
        disk.save(record_data);

        udho::session::record_data out{record_data.sessid()};
        disk.fetch(out);
        REQUIRE(out.size()==1);
        REQUIRE(out.get<std::string>("b")=="2");
    }

    SECTION("bad magic/version") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("x","y");
        disk.create(record_data);

        std::string session_file_name = boost::uuids::to_string(record_data.sessid()) + ".udho.session";
        udho::utils::filesystem::path expected_path = root / session_file_name;

        std::ofstream f(expected_path, std::ios::binary | std::ios::in | std::ios::out);
        uint32_t bogus = 0xAAAAAAAA;
        f.write(reinterpret_cast<char*>(&bogus), sizeof(bogus));
        f.close();

        udho::session::record_data broken{record_data.sessid()};
        CHECK_THROWS_AS(disk.fetch(broken), std::runtime_error);
    }

    SECTION("truncated file") {
        boost::uuids::uuid uuid = uuid_generator();
        std::string session_file_name = boost::uuids::to_string(uuid) + ".udho.session";
        udho::utils::filesystem::path expected_path = root / session_file_name;
        {
            std::ofstream t(expected_path, std::ios::binary|std::ios::trunc);
            t.write("1234", 4);
        }
        udho::session::record_data record_data{uuid};
        CHECK_THROWS_WITH(disk.fetch(record_data), "Corrupt file: too small");
    }


    SECTION("empty session") {
        udho::session::record_data empty{uuid_generator()};
        CHECK_NOTHROW(disk.create(empty));
        udho::session::record_data reloaded{empty.sessid()};
        CHECK_NOTHROW(disk.fetch(reloaded));
        CHECK(reloaded.size() == 0);
    }

    SECTION("binary data in value") {
        udho::session::record_data record_data{uuid_generator()};
        std::string blob = std::string("\0\1\2\xFF", 4);
        record_data.set("blob", blob);
        disk.create(record_data);
        udho::session::record_data out{record_data.sessid()};
        disk.fetch(out);
        CHECK(out.get<std::string>("blob") == blob);
    }

    SECTION("mismatched sessid") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("x","1");
        disk.create(record_data);

        auto wrong_id = uuid_generator();
        udho::utils::filesystem::path old = root / (to_string(record_data.sessid()) + ".udho.session");
        udho::utils::filesystem::path bad = root / (to_string(wrong_id) + ".udho.session");
        std::filesystem::rename(old, bad);

        udho::session::record_data broken{wrong_id};
        CHECK_THROWS_AS(disk.fetch(broken), std::runtime_error);
    }
}

TEST_CASE("session storage fs_mem", "[session][storage][fs_mem]") {
    REQUIRE(0 == 0);

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    udho::session::storage::mem_fs disk{root};

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

    SECTION("save-only roundtrip") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("a","1");
        disk.save(record_data);

        record_data.remove("a");
        record_data.set("b","2");
        disk.save(record_data);

        udho::session::record_data out{record_data.sessid()};
        disk.fetch(out);
        REQUIRE(out.size()==1);
        REQUIRE(out.get<std::string>("b")=="2");
    }

    SECTION("bad magic/version") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("x","y");
        disk.create(record_data);

        std::string session_file_name = boost::uuids::to_string(record_data.sessid()) + ".udho.session";
        udho::utils::filesystem::path expected_path = root / session_file_name;

        std::ofstream f(expected_path, std::ios::binary | std::ios::in | std::ios::out);
        uint32_t bogus = 0xAAAAAAAA;
        f.write(reinterpret_cast<char*>(&bogus), sizeof(bogus));
        f.close();

        udho::session::record_data broken{record_data.sessid()};
        CHECK_THROWS_AS(disk.fetch(broken), std::runtime_error);
    }

    SECTION("truncated file") {
        boost::uuids::uuid uuid = uuid_generator();
        std::string session_file_name = boost::uuids::to_string(uuid) + ".udho.session";
        udho::utils::filesystem::path expected_path = root / session_file_name;
        {
            std::ofstream t(expected_path, std::ios::binary|std::ios::trunc);
            t.write("1234", 4);
        }
        udho::session::record_data record_data{uuid};
        CHECK_THROWS_WITH(disk.fetch(record_data), "Corrupt file: too small");
    }

    SECTION("empty session") {
        udho::session::record_data empty{uuid_generator()};
        CHECK_NOTHROW(disk.create(empty));
        udho::session::record_data reloaded{empty.sessid()};
        CHECK_NOTHROW(disk.fetch(reloaded));
        CHECK(reloaded.size() == 0);
    }

    SECTION("binary data in value") {
        udho::session::record_data record_data{uuid_generator()};
        std::string blob = std::string("\0\1\2\xFF", 4);
        record_data.set("blob", blob);
        disk.create(record_data);
        udho::session::record_data out{record_data.sessid()};
        disk.fetch(out);
        CHECK(out.get<std::string>("blob") == blob);
    }

    SECTION("mismatched sessid") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("x","1");
        disk.create(record_data);

        auto wrong_id = uuid_generator();
        udho::utils::filesystem::path old = root / (to_string(record_data.sessid()) + ".udho.session");
        udho::utils::filesystem::path bad = root / (to_string(wrong_id) + ".udho.session");
        std::filesystem::rename(old, bad);

        udho::session::record_data broken{wrong_id};
        CHECK_THROWS_AS(disk.fetch(broken), std::runtime_error);
    }
}
