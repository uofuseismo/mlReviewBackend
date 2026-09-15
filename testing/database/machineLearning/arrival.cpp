#include <chrono>
#include <string>
#include "mlReview/database/machineLearning/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::MachineLearning::RealTime;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Database::MachineLearning::RealTime::Arrival",
          "[database][machineLearning][arrival]")
{
    Arrival arrival;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(arrival.haveIdentifier());
        REQUIRE_THROWS_AS(arrival.getIdentifier(), std::runtime_error);
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
    }

    SECTION("Setters and getters")
    {
        arrival.setIdentifier(1);
        arrival.setNetwork("UU");
        arrival.setStation("CTU");
        arrival.setChannels("HHZ", "HHN", "HHE");
        arrival.setLocationCode("01");
        arrival.setPhase(Arrival::Phase::S);
        arrival.setTime(1700000000.25);
        arrival.setResidual(0.05);

        REQUIRE(arrival.haveIdentifier());
        REQUIRE(arrival.getIdentifier() == 1);
        REQUIRE(arrival.getNetwork() == "UU");
        REQUIRE(arrival.getStation() == "CTU");
        REQUIRE(arrival.haveChannels());
        REQUIRE(arrival.getVerticalChannel() == "HHZ");
        auto nonVertical = arrival.getNonVerticalChannels();
        REQUIRE(nonVertical.has_value());
        REQUIRE(nonVertical->first == "HHN");
        REQUIRE(nonVertical->second == "HHE");
        REQUIRE(arrival.getLocationCode() == "01");
        REQUIRE(arrival.getPhase() == Arrival::Phase::S);
        REQUIRE(arrival.getTime() ==
                std::chrono::microseconds {1700000000250000});
        REQUIRE_THAT(*arrival.getResidual(), WithinAbs(0.05, 1.e-12));
    }

    SECTION("Single-component channel replaces three-component channels")
    {
        arrival.setChannels("HHZ", "HHN", "HHE");
        arrival.setChannels("EHZ");
        REQUIRE(arrival.getVerticalChannel() == "EHZ");
        REQUIRE_FALSE(arrival.getNonVerticalChannels().has_value());

        arrival.setChannels("ENZ", "", "");
        REQUIRE(arrival.getVerticalChannel() == "ENZ");
        REQUIRE_FALSE(arrival.getNonVerticalChannels().has_value());
    }

    SECTION("Time in microseconds")
    {
        arrival.setTime(std::chrono::microseconds {1700000000250000});
        REQUIRE(arrival.getTime() ==
                std::chrono::microseconds {1700000000250000});
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(arrival.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setNetwork("  "), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels("HZ"), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels("HHZ", "HN", "HHE"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setChannels("HHZ", "HHN", "HE"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setLocationCode(""), std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        arrival.setNetwork("UU");
        arrival.setPhase(Arrival::Phase::P);

        Arrival copy{arrival};
        REQUIRE(copy.getNetwork() == "UU");
        REQUIRE(copy.getPhase() == Arrival::Phase::P);

        Arrival moved{std::move(copy)};
        REQUIRE(moved.getNetwork() == "UU");

        arrival.clear();
        REQUIRE_FALSE(arrival.haveNetwork());
        REQUIRE_FALSE(arrival.havePhase());
        REQUIRE(moved.haveNetwork());
    }
}
