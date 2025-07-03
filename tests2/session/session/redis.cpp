#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/session/session.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/session/storage/redis.h>

#ifdef WITH_HIREDIS

TEST_CASE("session redis lazy", "[session][redis][lazy]") {
    using session_store = udho::session::catalogue<udho::session::storage::redis, udho::session::modes::lazy>;

    session_store store{"localhost", 6379};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");

    SECTION("Create new session") {
        store.storage()._command("DEL", "sess:" + boost::uuids::to_string(sessid));
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

    auto field_count = [&]() -> size_t {
        auto response = store.storage()._command("HLEN", "sess:" + boost::uuids::to_string(sessid));
        if (response.is_int()) {
            return static_cast<size_t>(response.as_int());
        }
        throw std::runtime_error("HLEN returned non-integer response");
    };

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::size_t keycount = field_count();
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
            CHECK(field_count() == keycount);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                note3["there"] = "here";
                CHECK(note3.exists("there"));
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(note["there"].as<std::string>() == "here");
            CHECK(note.exists("there"));
            CHECK(field_count() == keycount);
            note.unset("there");
            CHECK(!note.exists("there"));
            // Destruction of note will lead to serialization
        }
        CHECK(field_count() > keycount);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
            CHECK(!note.exists("there"));
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = field_count();
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(field_count() == before);
    }
}

TEST_CASE("session redis optimistic", "[session][redis][lazy]") {
    using session_store = udho::session::catalogue<udho::session::storage::redis, udho::session::modes::optimistic>;

    session_store store{"localhost", 6379};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");

    SECTION("Create new session") {
        store.storage()._command("DEL", "sess:" + boost::uuids::to_string(sessid));
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

    auto field_count = [&]() -> size_t {
        auto response = store.storage()._command("HLEN", "sess:" + boost::uuids::to_string(sessid));
        if (response.is_int()) {
            return static_cast<size_t>(response.as_int());
        }
        throw std::runtime_error("HLEN returned non-integer response");
    };

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::size_t keycount = field_count();
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
            CHECK(field_count() == keycount);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                note3["there"] = "here";
                CHECK(note3.exists("there"));
                CHECK(note.dirty());
                // Destruction of note3 will not lead to serialization
            }
            CHECK(note["there"].as<std::string>() == "here");
            CHECK(note.exists("there"));
            CHECK(field_count() == keycount);
            note.unset("there");
            CHECK(!note.exists("there"));
            // Destruction of note will lead to serialization
        }
        CHECK(field_count() > keycount);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
            CHECK(!note.exists("there"));
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = field_count();
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(field_count() == before);
    }
}

TEST_CASE("session redis immediate", "[session][redis][immediate]") {
    using session_store = udho::session::catalogue<udho::session::storage::redis, udho::session::modes::immediate>;

    session_store store{"localhost", 6379};

    boost::uuids::random_generator uuid_generator;
    boost::uuids::string_generator uuid_string_generator;
    boost::uuids::uuid sessid = uuid_string_generator("f18e86c9-c370-4dac-878a-38419bfa4087");

    SECTION("Create new session") {
        store.storage()._command("DEL", "sess:" + boost::uuids::to_string(sessid));
        REQUIRE(!store.storage().exists(sessid));
        {
            udho::session::note note = store.borrow(sessid);
            note["hello"] = "world";
            {
                auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "hello");
                REQUIRE(response.is_int());
                CHECK(response.as_int());
            } {
                auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "year");
                REQUIRE(response.is_int());
                CHECK(!response.as_int());
            }
            note["year"]  = 2018;
            {
                auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "hello");
                REQUIRE(response.is_int());
                CHECK(response.as_int());
            } {
                auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "year");
                REQUIRE(response.is_int());
                CHECK(response.as_int());
            }
            CHECK(!note.dirty());
            {
                udho::session::note note2 = store.borrow(sessid);
                note["planet"] = "earth";
                note["age"]  = 72633552;
                {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "hello");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "year");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "planet");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "age");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                }
                CHECK(!note.dirty());
            }
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

    auto field_count = [&]() -> size_t {
        auto response = store.storage()._command("HLEN", "sess:" + boost::uuids::to_string(sessid));
        if (response.is_int()) {
            return static_cast<size_t>(response.as_int());
        }
        throw std::runtime_error("HLEN returned non-integer response");
    };

    SECTION("Fetch existing session") {
        REQUIRE(store.storage().exists(sessid));
        std::size_t keycount = field_count();
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
                {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "hello");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "year");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "planet");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "age");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "first_name");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                }
                CHECK(!note.dirty());
            }
            CHECK(field_count() == keycount+1);
            {
                udho::session::note note3 = store.borrow(sessid);
                note3["last_name"] = "Basu";
                {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "hello");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "year");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                }            {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "planet");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "age");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "first_name");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                } {
                    auto response = store.storage()._command("HEXISTS", "sess:" + boost::uuids::to_string(sessid), "last_name");
                    REQUIRE(response.is_int());
                    CHECK(response.as_int());
                }
                note3["there"] = "here";
                CHECK(note3.exists("there"));
                CHECK(!note.dirty());
            }
            CHECK(note["there"].as<std::string>() == "here");
            CHECK(note.exists("there"));
            CHECK(field_count() == keycount+3);
            note.unset("there");
            CHECK(!note.exists("there"));
            CHECK(field_count() == keycount+2);
        }
        CHECK(field_count() > keycount);
        {
            udho::session::note note = store.borrow(sessid);
            CHECK(note["hello"].as<std::string>() == "world");
            CHECK(note["year"].as<std::uint16_t>() == 2018);
            CHECK(note["planet"].as<std::string>() == "earth");
            CHECK(note["age"].as<std::uint32_t>() == 72633552);
            CHECK(note["first_name"].as<std::string>() == "Neel");
            CHECK(note["last_name"].as<std::string>() == "Basu");
            CHECK(!note.dirty());
            CHECK(!note.exists("there"));
        }
    }

    SECTION("no-op borrow does not serialize") {
        auto before = field_count();
        {
            udho::session::note note = store.borrow(sessid);
            (void) note["hello"].as<std::string>();
            CHECK(!note.dirty());
        }
        CHECK(field_count() == before);
    }
}

#endif  // WITH_HIREDIS
