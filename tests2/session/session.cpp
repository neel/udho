#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/session/session.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/fs_mem.h>

TEST_CASE("session fs lazy", "[session][fs][lazy]") {
    using session_store = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    session_store store{root};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");
    udho::utils::filesystem::path sesspath = store.storage().path(sessid);

    SECTION("Create new session") {
        if(udho::utils::filesystem::exists(sesspath)) {
            udho::utils::filesystem::remove(sesspath);
        }
        REQUIRE(!store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            note["hello"] = "world";
            note["year"]  = 2018;
            CHECK(note.dirty());
            {
                udho::session::note note2 = store.borrow(sessid);
                note["planet"] = "earth";
                note["age"]  = 72633552;
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            // Destruction of note will lead to serialization
        }
        REQUIRE(store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(!note.dirty());
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());
        }
    }

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::uintmax_t filesize = udho::utils::filesystem::file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());

            {
                udho::session::note note2 = store.borrow(sessid);
                note2["first_name"] = "Neel";
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            // Destruction of note will lead to serialization
        }
        CHECK(udho::utils::filesystem::file_size(sesspath) > filesize);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(file_size(sesspath) == before);
    }

}

TEST_CASE("session fs_mem lazy", "[session][fs_mem][lazy]") {
    using session_store = udho::session::catalogue<udho::session::storage::mem_fs, udho::session::modes::lazy>;

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    session_store store{root};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");
    udho::utils::filesystem::path sesspath = store.storage().path(sessid);

    SECTION("Create new session") {
        if(udho::utils::filesystem::exists(sesspath)) {
            udho::utils::filesystem::remove(sesspath);
        }
        REQUIRE(!store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            note["hello"] = "world";
            note["year"]  = 2018;
            CHECK(note.dirty());
            {
                udho::session::note note2 = store.borrow(sessid);
                note["planet"] = "earth";
                note["age"]  = 72633552;
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            // Destruction of note will lead to serialization
        }
        REQUIRE(store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(!note.dirty());
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());
        }
    }

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::uintmax_t filesize = udho::utils::filesystem::file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());

            {
                udho::session::note note2 = store.borrow(sessid);
                note2["first_name"] = "Neel";
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            // Destruction of note will lead to serialization
        }
        CHECK(udho::utils::filesystem::file_size(sesspath) > filesize);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(file_size(sesspath) == before);
    }

}


TEST_CASE("session fs optimistic", "[session][fs][optimistic]") {
    using session_store = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::optimistic>;

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    session_store store{root};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");
    udho::utils::filesystem::path sesspath = store.storage().path(sessid);

    SECTION("Create new session") {
        if(udho::utils::filesystem::exists(sesspath)) {
            udho::utils::filesystem::remove(sesspath);
        }
        REQUIRE(!store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            note["hello"] = "world";
            note["year"]  = 2018;
            CHECK(note.dirty());
            {
                udho::session::note note2 = store.borrow(sessid);
                note["planet"] = "earth";
                note["age"]  = 72633552;
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            // Destruction of note will lead to serialization
        }
        REQUIRE(store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(!note.dirty());
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());
        }
    }

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::uintmax_t filesize = udho::utils::filesystem::file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());

            {
                udho::session::note note2 = store.borrow(sessid);
                note2["first_name"] = "Neel";
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            // Destruction of note will lead to serialization
        }
        CHECK(udho::utils::filesystem::file_size(sesspath) > filesize);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(file_size(sesspath) == before);
    }

}

TEST_CASE("session fs_mem optimistic", "[session][fs_mem][optimistic]") {
    using session_store = udho::session::catalogue<udho::session::storage::mem_fs, udho::session::modes::optimistic>;

    udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
    session_store store{root};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");
    udho::utils::filesystem::path sesspath = store.storage().path(sessid);

    SECTION("Create new session") {
        if(udho::utils::filesystem::exists(sesspath)) {
            udho::utils::filesystem::remove(sesspath);
        }
        REQUIRE(!store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            note["hello"] = "world";
            note["year"]  = 2018;
            CHECK(note.dirty());
            {
                udho::session::note note2 = store.borrow(sessid);
                note["planet"] = "earth";
                note["age"]  = 72633552;
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            // Destruction of note will lead to serialization
        }
        REQUIRE(store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(!note.dirty());
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());
        }
    }

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::uintmax_t filesize = udho::utils::filesystem::file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(!note.dirty());

            {
                udho::session::note note2 = store.borrow(sessid);
                note2["first_name"] = "Neel";
                CHECK(note.dirty());
                // Destruction of note2 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(udho::utils::filesystem::file_size(sesspath) == filesize);
            // Destruction of note will lead to serialization
        }
        CHECK(udho::utils::filesystem::file_size(sesspath) > filesize);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = file_size(sesspath);
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(file_size(sesspath) == before);
    }

}
