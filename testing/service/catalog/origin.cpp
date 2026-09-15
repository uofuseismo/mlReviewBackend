#include <chrono>
#include <string>
#include <vector>
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Service::Catalog;
using Catch::Matchers::WithinAbs;

namespace
{
Arrival makeArrival(const std::string &station)
{
    Arrival arrival;
    arrival.setNetwork("UU");
    arrival.setStation(station);
    arrival.setLocationCode("01");
    arrival.setPhase("P");
    arrival.setTime(1700000001.0);
    return arrival;
}
}

TEST_CASE("MLReview::Service::Catalog::Origin", "[service][catalog][origin]")
{
    Origin origin;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(origin.haveTime());
        REQUIRE_FALSE(origin.haveLatitude());
        REQUIRE_FALSE(origin.haveLongitude());
        REQUIRE_FALSE(origin.haveDepth());
        REQUIRE_THROWS_AS(origin.getTime(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getDepth(), std::runtime_error);
        REQUIRE_FALSE(origin.havePreferredMagnitude());
        REQUIRE(origin.getArrivalsReference().empty());
        REQUIRE(origin.getEventType() == Origin::EventType::Unknown);
    }

    SECTION("Setters and getters")
    {
        origin.setTime(1700000000.25);
        origin.setLatitude(-90);
        origin.setLongitude(-111.9);
        origin.setDepth(-8600);
        origin.setEventType(Origin::EventType::QuarryBlast);

        REQUIRE(origin.getTime() ==
                std::chrono::microseconds {1700000000250000});
        REQUIRE_THAT(origin.getLatitude(), WithinAbs(-90, 1.e-12));
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-111.9, 1.e-12));
        REQUIRE_THAT(origin.getDepth(), WithinAbs(-8600, 1.e-12));
        REQUIRE(origin.getEventType() == Origin::EventType::QuarryBlast);
    }

    SECTION("Longitude of 180 wraps to -180")
    {
        origin.setLongitude(180);
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-180, 1.e-10));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(origin.setLatitude(90.5), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(-8601), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(800001), std::invalid_argument);
    }

    SECTION("Incomplete arrivals are skipped")
    {
        Arrival noPhase;
        noPhase.setNetwork("UU");
        noPhase.setStation("ELU");
        noPhase.setLocationCode("01");
        noPhase.setTime(1700000002.0);

        Arrival noLocationCode;
        noLocationCode.setNetwork("UU");
        noLocationCode.setStation("MOUT");
        noLocationCode.setPhase("S");
        noLocationCode.setTime(1700000003.0);

        origin.setArrivals({makeArrival("CTU"), noPhase, noLocationCode});
        const auto &arrivals = origin.getArrivalsReference();
        REQUIRE(arrivals.size() == 1);
        REQUIRE(arrivals.at(0).getStation() == "CTU");
    }

    SECTION("Copy, move, and clear")
    {
        origin.setLatitude(40.5);
        origin.setArrivals({makeArrival("CTU")});

        Origin copy{origin};
        REQUIRE(copy.haveLatitude());
        REQUIRE(copy.getArrivalsReference().size() == 1);

        Origin moved{std::move(copy)};
        REQUIRE(moved.haveLatitude());

        origin.clear();
        REQUIRE_FALSE(origin.haveLatitude());
        REQUIRE(origin.getArrivalsReference().empty());
        REQUIRE(moved.haveLatitude());
    }
}
