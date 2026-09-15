#include <chrono>
#include <string>
#include "mlReview/service/catalog/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Service::Catalog;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Service::Catalog::Arrival", "[service][catalog][arrival]")
{
    Arrival arrival;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(arrival.haveNetwork());
        REQUIRE_FALSE(arrival.haveStation());
        REQUIRE_FALSE(arrival.haveChannels());
        REQUIRE_FALSE(arrival.haveLocationCode());
        REQUIRE_FALSE(arrival.havePhase());
        REQUIRE_FALSE(arrival.haveTime());
        REQUIRE_THROWS_AS(arrival.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getLocationCode(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getPhase(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getTime(), std::runtime_error);
        REQUIRE_FALSE(arrival.getResidual().has_value());
        REQUIRE_FALSE(arrival.getDistance().has_value());
        REQUIRE_FALSE(arrival.getAzimuth().has_value());
    }

    SECTION("Setters and getters")
    {
        arrival.setNetwork("UU");
        arrival.setStation("ELU");
        arrival.setChannels("HHZ", "HHN", "HHE");
        arrival.setLocationCode("01");
        arrival.setPhase("P");
        arrival.setTime(1700000000.25);
        arrival.setResidual(-0.2);
        arrival.setDistance(1000);
        arrival.setAzimuth(359);

        REQUIRE(arrival.getNetwork() == "UU");
        REQUIRE(arrival.getStation() == "ELU");
        REQUIRE(arrival.getVerticalChannel() == "HHZ");
        auto nonVertical = arrival.getNonVerticalChannels();
        REQUIRE(nonVertical.has_value());
        REQUIRE(nonVertical->first == "HHN");
        REQUIRE(nonVertical->second == "HHE");
        REQUIRE(arrival.getLocationCode() == "01");
        REQUIRE(arrival.getPhase() == "P");
        REQUIRE(arrival.getTime() ==
                std::chrono::microseconds {1700000000250000});
        REQUIRE_THAT(*arrival.getResidual(), WithinAbs(-0.2, 1.e-12));
        REQUIRE_THAT(*arrival.getDistance(), WithinAbs(1000, 1.e-12));
        REQUIRE_THAT(*arrival.getAzimuth(), WithinAbs(359, 1.e-12));
    }

    SECTION("Vertical-only channel")
    {
        arrival.setChannels("HHZ", "HHN", "HHE");
        arrival.setChannels("EHZ");
        REQUIRE(arrival.getVerticalChannel() == "EHZ");
        REQUIRE_FALSE(arrival.getNonVerticalChannels().has_value());

        arrival.setChannels("ENZ", "", "");
        REQUIRE(arrival.getVerticalChannel() == "ENZ");
        REQUIRE_FALSE(arrival.getNonVerticalChannels().has_value());

        REQUIRE_THROWS_AS(arrival.setChannels(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels("HZ"), std::invalid_argument);
    }

    SECTION("Zero distance and azimuth are valid")
    {
        arrival.setDistance(0);
        arrival.setAzimuth(0);
        REQUIRE(arrival.getDistance().has_value());
        REQUIRE(arrival.getAzimuth().has_value());
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(arrival.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setStation(" "), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels("HZ", "", ""),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setLocationCode(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setPhase(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setDistance(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setAzimuth(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setAzimuth(360), std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        arrival.setStation("ELU");
        arrival.setDistance(5);

        Arrival copy{arrival};
        REQUIRE(copy.getStation() == "ELU");
        REQUIRE(copy.getDistance().has_value());

        Arrival moved{std::move(copy)};
        REQUIRE(moved.getStation() == "ELU");

        arrival.clear();
        REQUIRE_FALSE(arrival.haveStation());
        REQUIRE_FALSE(arrival.getDistance().has_value());
        REQUIRE(moved.haveStation());
    }
}
