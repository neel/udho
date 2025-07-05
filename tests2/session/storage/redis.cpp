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
#include <udho/utils/filesystem.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#ifdef WITH_HIREDIS

TEST_CASE("session storage redis", "[session][storage][redis]") {
    REQUIRE(0 == 0);
    udho::session::storage::redis redis_store("localhost", 6379);
    boost::uuids::random_generator uuid_generator;

    SECTION("record storage and retrieval") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("name", "Neel Basu");
        record_data.set("weight", 48);

        CHECK_NOTHROW(redis_store.create(record_data));

        udho::session::record_data record{record_data.sessid()};
        CHECK_NOTHROW(redis_store.fetch(record));

        CHECK(record.size() == record_data.size());
        CHECK(record.get<std::string>("name") == record_data.get<std::string>("name"));
        CHECK(record.get<int>("weight") == record_data.get<int>("weight"));
    }

    SECTION("save-only roundtrip") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("a","1");
        redis_store.save(record_data, false);

        record_data.remove("a");
        record_data.set("b","2");
        redis_store.save(record_data, false);

        udho::session::record_data out{record_data.sessid()};
        redis_store.fetch(out);
        REQUIRE(out.size() == 1);
        REQUIRE(out.get<std::string>("b") == "2");
    }

    SECTION("empty session") {
        udho::session::record_data empty{uuid_generator()};
        CHECK_NOTHROW(redis_store.create(empty));
        udho::session::record_data reloaded{empty.sessid()};
        CHECK_NOTHROW(redis_store.fetch(reloaded));
        CHECK(reloaded.size() == 0);
    }

    SECTION("binary data in value") {
        udho::session::record_data record_data{uuid_generator()};
        std::string blob = std::string("\0\1\2\xFF", 4);
        record_data.set("blob", blob);
        redis_store.create(record_data);
        udho::session::record_data out{record_data.sessid()};
        redis_store.fetch(out);
        CHECK(out.get<std::string>("blob") == blob);
    }

    SECTION("mismatched sessid") {
        udho::session::record_data record_data{uuid_generator()};
        record_data.set("x","1");
        redis_store.create(record_data);

        auto wrong_id = uuid_generator();
        udho::session::record_data broken{wrong_id};
        CHECK_THROWS_AS(redis_store.fetch(broken), std::runtime_error);
    }
}

TEST_CASE("session storage redis (optimistic/CAS)", "[session][storage][redis][optimistic]") {
    udho::session::storage::redis redis_store("localhost", 6379);
    boost::uuids::random_generator ugen;

    SECTION("optimistic save succeeds when no one else modified") {
        udho::session::record_data rec1{ugen()};
        rec1.set("foo", "bar");
        CHECK_NOTHROW(redis_store.create(rec1));

        CHECK_NOTHROW(redis_store.save(rec1, true));
        udho::session::record_data out{rec1.sessid()};
        CHECK_NOTHROW(redis_store.fetch(out));
        CHECK(out.get<std::string>("foo") == "bar");
        CHECK(out.revision() == rec1.revision());
    }

    SECTION("optimistic save throws conflict if another writer updated first") {
        udho::session::record_data original{ugen()};
        original.set("x", "1");
        redis_store.create(original);

        udho::session::record_data clientA{original.sessid()};
        redis_store.fetch(clientA);
        REQUIRE(clientA.revision() == 1);

        udho::session::record_data clientB{original.sessid()};
        redis_store.fetch(clientB);
        REQUIRE(clientB.revision() == 1);

        clientA.set("x", "2");
        CHECK_NOTHROW(redis_store.save(clientA, true));
        CHECK(clientA.revision() == 2);

        clientB.set("y", "alpha");
        CHECK_THROWS_AS(redis_store.save(clientB, true), udho::session::errors::conflict);
    }

    SECTION("mixed modes: optimistic=false still overwrites on conflict") {
        udho::session::record_data rA{ugen()};
        rA.set("m", "initial");
        redis_store.create(rA);

        udho::session::record_data A{rA.sessid()};
        redis_store.fetch(A);
        A.set("m", "A1");
        CHECK_NOTHROW(redis_store.save(A, false));

        udho::session::record_data B{rA.sessid()};
        redis_store.fetch(B);
        REQUIRE(B.revision() == 2);
        B.set("m", "B1");
        CHECK_NOTHROW(redis_store.save(B, false));

        udho::session::record_data final{rA.sessid()};
        redis_store.fetch(final);
        CHECK(final.get<std::string>("m") == "B1");
    }

    SECTION("revision increment on save") {
        udho::session::record_data rec{ugen()};
        rec.set("counter", "1");
        redis_store.create(rec);
        uint64_t initial_rev = rec.revision();

        redis_store.save(rec, false);
        CHECK(rec.revision() == initial_rev + 1);

        redis_store.save(rec, false);
        CHECK(rec.revision() == initial_rev + 2);
    }

    SECTION("created timestamp preserved") {
        udho::session::record_data rec{ugen()};
        auto create_time = rec.created();
        rec.set("test", "value");
        redis_store.create(rec);

        udho::session::record_data loaded{rec.sessid()};
        redis_store.fetch(loaded);
        CHECK(loaded.created() == create_time);
    }
}

#endif  // WITH_HIREDIS
