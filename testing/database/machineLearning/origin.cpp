#include <chrono>
#include <string>
#include <vector>
#include "mlReview/database/machineLearning/origin.hpp"
#include "mlReview/database/machineLearning/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::MachineLearning::RealTime;
using Catch::Matchers::WithinAbs;

namespace
{
Arrival makeArrival(const std::string &station, const Arrival::Phase phase)
{
    Arrival arrival;
    arrival.setNetwork("UU");
    arrival.setStation(station);
    arrival.setChannels("HHZ", "HHN", "HHE");
    arrival.setLocationCode("01");
    arrival.setPhase(phase);
    arrival.setTime(1700000001.0);
    return arrival;
}
}

TEST_CASE("MLReview::Database::MachineLearning::RealTime::Origin",
          "[database][machineLearning][origin]")
{
    Origin origin;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(origin.haveIdentifier());
        REQUIRE_FALSE(origin.haveTime());
        REQUIRE_FALSE(origin.haveLatitude());
        REQUIRE_FALSE(origin.haveLongitude());
        REQUIRE_FALSE(origin.haveDepth());
        REQUIRE_THROWS_AS(origin.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getTime(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getDepth(), std::runtime_error);
        REQUIRE(origin.getArrivals().empty());
        REQUIRE(origin.getArrivalsReference().empty());
        REQUIRE_FALSE(origin.getReviewStatus().has_value());
        REQUIRE_FALSE(origin.getAlgorithm().has_value());
    }

    SECTION("Setters and getters")
    {
        origin.setIdentifier(9);
        origin.setTime(1700000000.25);
        origin.setLatitude(44.5);
        origin.setLongitude(-110.7);
        origin.setDepth(5000);
        origin.setReviewStatus(Origin::ReviewStatus::Human);
        origin.setAlgorithm("massociate");

        REQUIRE(origin.getIdentifier() == 9);
        REQUIRE(origin.getTime() ==
                std::chrono::microseconds {1700000000250000});
        REQUIRE_THAT(origin.getLatitude(), WithinAbs(44.5, 1.e-12));
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-110.7, 1.e-12));
        REQUIRE_THAT(origin.getDepth(), WithinAbs(5000, 1.e-12));
        REQUIRE(*origin.getReviewStatus() == Origin::ReviewStatus::Human);
        REQUIRE(*origin.getAlgorithm() == "massociate");
    }

    SECTION("Longitude is wrapped to [-180,180)")
    {
        origin.setLongitude(-190);
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(170, 1.e-10));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(origin.setLatitude(90.1), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(-8601), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(800001), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setAlgorithm(""), std::invalid_argument);
    }

    SECTION("Incomplete arrivals are skipped")
    {
        Arrival noChannels;
        noChannels.setNetwork("UU");
        noChannels.setStation("ELU");
        noChannels.setLocationCode("01");
        noChannels.setPhase(Arrival::Phase::P);
        noChannels.setTime(1700000002.0);

        origin.setArrivals({makeArrival("CTU", Arrival::Phase::P),
                            makeArrival("CTU", Arrival::Phase::S),
                            noChannels});
        REQUIRE(origin.getArrivals().size() == 2);
        REQUIRE(origin.getArrivalsReference().size() == 2);
    }

    SECTION("Duplicate network/station/phase arrivals are skipped")
    {
        origin.setArrivals({makeArrival("CTU", Arrival::Phase::P),
                            makeArrival("CTU", Arrival::Phase::P)});
        REQUIRE(origin.getArrivalsReference().size() == 1);
    }

    SECTION("Copy, move, and clear")
    {
        origin.setIdentifier(9);
        origin.setArrivals({makeArrival("CTU", Arrival::Phase::P)});

        Origin copy{origin};
        REQUIRE(copy.getIdentifier() == 9);
        REQUIRE(copy.getArrivalsReference().size() == 1);

        Origin moved{std::move(copy)};
        REQUIRE(moved.getIdentifier() == 9);

        origin.clear();
        REQUIRE_FALSE(origin.haveIdentifier());
        REQUIRE(origin.getArrivalsReference().empty());
        REQUIRE(moved.haveIdentifier());
    }
}
