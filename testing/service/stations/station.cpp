#include <chrono>
#include <string>
#include "mlReview/service/stations/station.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Service::Stations;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Service::Stations::Station", "[service][stations]")
{
    Station station;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(station.haveNetwork());
        REQUIRE_FALSE(station.haveName());
        REQUIRE_FALSE(station.haveLatitude());
        REQUIRE_FALSE(station.haveLongitude());
        REQUIRE_FALSE(station.haveElevation());
        REQUIRE_FALSE(station.haveOnOffDate());
        REQUIRE_THROWS_AS(station.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getName(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getElevation(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getOnDate(), std::runtime_error);
        REQUIRE_THROWS_AS(station.getOffDate(), std::runtime_error);
        REQUIRE(station.getDescription().empty());
    }

    SECTION("Setters and getters")
    {
        station.setNetwork("UU");
        station.setName("CTU");
        station.setLatitude(40.7);
        station.setLongitude(-111.8);
        station.setElevation(1500);
        station.setDescription("Salt Lake City, UT, USA");
        station.setOnOffDate({std::chrono::seconds {1000},
                              std::chrono::seconds {2000}});

        REQUIRE(station.getNetwork() == "UU");
        REQUIRE(station.getName() == "CTU");
        REQUIRE_THAT(station.getLatitude(), WithinAbs(40.7, 1.e-12));
        REQUIRE_THAT(station.getLongitude(), WithinAbs(-111.8, 1.e-12));
        REQUIRE_THAT(station.getElevation(), WithinAbs(1500, 1.e-12));
        REQUIRE(station.getDescription() == "Salt Lake City, UT, USA");
        REQUIRE(station.getOnDate() == std::chrono::seconds {1000});
        REQUIRE(station.getOffDate() == std::chrono::seconds {2000});
    }

    SECTION("Longitude is wrapped to [-180,180)")
    {
        station.setLongitude(248.2);
        REQUIRE_THAT(station.getLongitude(), WithinAbs(-111.8, 1.e-10));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(station.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(station.setName(""), std::invalid_argument);
        REQUIRE_THROWS_AS(station.setLatitude(91), std::invalid_argument);
        REQUIRE_THROWS_AS(station.setElevation(8601), std::invalid_argument);
        REQUIRE_THROWS_AS(station.setElevation(-10001),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(station.setOnOffDate({std::chrono::seconds {2000},
                                                std::chrono::seconds {1000}}),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(station.setOnOffDate({std::chrono::seconds {1000},
                                                std::chrono::seconds {1000}}),
                          std::invalid_argument);
    }

    SECTION("Is local")
    {
        station.setName("CTU");
        station.setNetwork("UU");
        REQUIRE(station.isLocal());
        station.setNetwork("WY");
        REQUIRE(station.isLocal());
        station.setNetwork("US");
        REQUIRE_FALSE(station.isLocal());

        station.setNetwork("NP");
        REQUIRE_FALSE(station.isLocal());
        station.setName("7234");
        REQUIRE(station.isLocal());
    }

    SECTION("Copy, move, and clear")
    {
        station.setNetwork("UU");
        station.setElevation(1500);

        Station copy{station};
        REQUIRE(copy.getNetwork() == "UU");
        REQUIRE(copy.haveElevation());

        Station moved{std::move(copy)};
        REQUIRE(moved.getNetwork() == "UU");

        station.clear();
        REQUIRE_FALSE(station.haveNetwork());
        REQUIRE_FALSE(station.haveElevation());
        REQUIRE(moved.haveNetwork());
    }
}
