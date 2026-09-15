#include <chrono>
#include <string>
#include "mlReview/database/aqms/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::AQMS;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Database::AQMS::Arrival", "[database][aqms][arrival]")
{
    Arrival arrival;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(arrival.haveAuthority());
        REQUIRE_FALSE(arrival.haveStation());
        REQUIRE_FALSE(arrival.haveTime());
        REQUIRE_THROWS_AS(arrival.getAuthority(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(arrival.getTime(), std::runtime_error);
        REQUIRE_FALSE(arrival.getNetwork().has_value());
        REQUIRE_FALSE(arrival.getSEEDChannel().has_value());
        REQUIRE_FALSE(arrival.getLocationCode().has_value());
        REQUIRE_FALSE(arrival.getIdentifier().has_value());
        REQUIRE_FALSE(arrival.getPhase().has_value());
        REQUIRE_FALSE(arrival.getQuality().has_value());
        REQUIRE_FALSE(arrival.getSubSource().has_value());
        REQUIRE_FALSE(arrival.getReviewFlag().has_value());
        REQUIRE(arrival.getFirstMotion() == Arrival::FirstMotion::Unknown);
    }

    SECTION("Setters and getters")
    {
        arrival.setAuthority("uu");
        arrival.setStation("ahid");
        arrival.setTime(1700000000.25);
        arrival.setNetwork("wy");
        arrival.setSEEDChannel("hhz");
        arrival.setLocationCode("01");
        arrival.setIdentifier(1234);
        arrival.setPhase("P");
        arrival.setQuality(0.75);
        arrival.setSubSource("RT1");
        arrival.setReviewFlag(Arrival::ReviewFlag::Human);
        arrival.setFirstMotion(Arrival::FirstMotion::Up);

        REQUIRE(arrival.getAuthority() == "UU");
        REQUIRE(arrival.getStation() == "AHID");
        REQUIRE_THAT(arrival.getTime(), WithinAbs(1700000000.25, 1.e-6));
        REQUIRE(*arrival.getNetwork() == "WY");
        REQUIRE(*arrival.getSEEDChannel() == "HHZ");
        REQUIRE(*arrival.getLocationCode() == "01");
        REQUIRE(*arrival.getIdentifier() == 1234);
        REQUIRE(*arrival.getPhase() == "P");
        REQUIRE_THAT(*arrival.getQuality(), WithinAbs(0.75, 1.e-12));
        REQUIRE(*arrival.getSubSource() == "RT1");
        REQUIRE(*arrival.getReviewFlag() == Arrival::ReviewFlag::Human);
        REQUIRE(arrival.getFirstMotion() == Arrival::FirstMotion::Up);
    }

    SECTION("Whitespace is removed")
    {
        arrival.setAuthority(" u u ");
        arrival.setStation(" ahid ");
        arrival.setNetwork("w y");
        arrival.setSEEDChannel(" hhz");
        arrival.setPhase(" P ");
        REQUIRE(arrival.getAuthority() == "UU");
        REQUIRE(arrival.getStation() == "AHID");
        REQUIRE(*arrival.getNetwork() == "WY");
        REQUIRE(*arrival.getSEEDChannel() == "HHZ");
        REQUIRE(*arrival.getPhase() == "P");
        REQUIRE_THROWS_AS(arrival.setStation("   "), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setPhase("   "), std::invalid_argument);
        // Six characters once the blanks are gone
        REQUIRE_NOTHROW(arrival.setStation(" ABCDEF "));
    }

    SECTION("Time in microseconds")
    {
        arrival.setTime(std::chrono::microseconds {1700000000250000});
        REQUIRE_THAT(arrival.getTime(), WithinAbs(1700000000.25, 1.e-6));
    }

    SECTION("Quality bounds are inclusive")
    {
        arrival.setQuality(0);
        REQUIRE_THAT(*arrival.getQuality(), WithinAbs(0, 1.e-12));
        arrival.setQuality(1);
        REQUIRE_THAT(*arrival.getQuality(), WithinAbs(1, 1.e-12));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(arrival.setAuthority(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setAuthority("ABCDEFGHIJKLMNOP"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setStation("ABCDEFG"), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setNetwork("ABCDEFGHI"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setSEEDChannel(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setSEEDChannel("HHZZ"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setLocationCode("001"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setPhase(""), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setPhase("PPPPPPPPP"), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setQuality(-0.1), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setQuality(1.1), std::invalid_argument);
        REQUIRE_THROWS_AS(arrival.setSubSource("123456789"),
                          std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        arrival.setStation("AHID");
        arrival.setIdentifier(5);

        Arrival copy{arrival};
        REQUIRE(copy.getStation() == "AHID");
        REQUIRE(*copy.getIdentifier() == 5);

        Arrival moved{std::move(copy)};
        REQUIRE(moved.getStation() == "AHID");

        arrival.clear();
        REQUIRE_FALSE(arrival.haveStation());
        REQUIRE_FALSE(arrival.getIdentifier().has_value());
        REQUIRE(moved.haveStation());
    }
}
