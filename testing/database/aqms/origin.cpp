#include <chrono>
#include <string>
#include "mlReview/database/aqms/origin.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::AQMS;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Database::AQMS::Origin", "[database][aqms][origin]")
{
    Origin origin;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(origin.haveAuthority());
        REQUIRE_FALSE(origin.haveIdentifier());
        REQUIRE_FALSE(origin.haveEventIdentifier());
        REQUIRE_FALSE(origin.haveTime());
        REQUIRE_FALSE(origin.haveLatitude());
        REQUIRE_FALSE(origin.haveLongitude());
        REQUIRE_THROWS_AS(origin.getAuthority(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getEventIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getTime(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLongitude(), std::runtime_error);
        REQUIRE_FALSE(origin.isBogus());
        REQUIRE_FALSE(origin.getPreferredMagnitudeIdentifier().has_value());
        REQUIRE_FALSE(origin.getPreferredMechanismIdentifier().has_value());
        REQUIRE_FALSE(origin.getDepth().has_value());
        REQUIRE_FALSE(origin.getGeographicType().has_value());
        REQUIRE_FALSE(origin.getAlgorithm().has_value());
        REQUIRE_FALSE(origin.getSubSource().has_value());
        REQUIRE_FALSE(origin.getGap().has_value());
        REQUIRE_FALSE(origin.getDistanceToNearestStation().has_value());
        REQUIRE_FALSE(origin.getWeightedRootMeanSquaredError().has_value());
        REQUIRE_FALSE(origin.getReviewFlag().has_value());
    }

    SECTION("Setters and getters")
    {
        origin.setAuthority("uu");
        origin.setIdentifier(5);
        origin.setEventIdentifier(6);
        origin.setTime(1700000000.5);
        origin.setLatitude(40.5);
        origin.setLongitude(-111.9);
        origin.setPreferredMagnitudeIdentifier(7);
        origin.setPreferredMechanismIdentifier(8);
        origin.setDepth(10.2);
        origin.setGeographicType(Origin::GeographicType::Regional);
        origin.setAlgorithm("hypoinverse");
        origin.setSubSource("RT1");
        origin.setGap(180);
        origin.setDistanceToNearestStation(3.2);
        origin.setWeightedRootMeanSquaredError(0.12);
        origin.setReviewFlag(Origin::ReviewFlag::Finalized);

        REQUIRE(origin.getAuthority() == "UU");
        REQUIRE(origin.getIdentifier() == 5);
        REQUIRE(origin.getEventIdentifier() == 6);
        REQUIRE_THAT(origin.getTime(), WithinAbs(1700000000.5, 1.e-6));
        REQUIRE_THAT(origin.getLatitude(), WithinAbs(40.5, 1.e-12));
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-111.9, 1.e-12));
        REQUIRE(*origin.getPreferredMagnitudeIdentifier() == 7);
        REQUIRE(*origin.getPreferredMechanismIdentifier() == 8);
        REQUIRE_THAT(*origin.getDepth(), WithinAbs(10.2, 1.e-12));
        REQUIRE(*origin.getGeographicType() ==
                Origin::GeographicType::Regional);
        REQUIRE(*origin.getAlgorithm() == "hypoinverse");
        REQUIRE(*origin.getSubSource() == "RT1");
        REQUIRE_THAT(*origin.getGap(), WithinAbs(180, 1.e-12));
        REQUIRE_THAT(*origin.getDistanceToNearestStation(),
                     WithinAbs(3.2, 1.e-12));
        REQUIRE_THAT(*origin.getWeightedRootMeanSquaredError(),
                     WithinAbs(0.12, 1.e-12));
        REQUIRE(*origin.getReviewFlag() == Origin::ReviewFlag::Finalized);
    }

    SECTION("Time in microseconds")
    {
        origin.setTime(std::chrono::microseconds {1700000000500000});
        REQUIRE_THAT(origin.getTime(), WithinAbs(1700000000.5, 1.e-6));
    }

    SECTION("Longitude is wrapped to [-180,180)")
    {
        origin.setLongitude(250);
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-110, 1.e-10));
        origin.setLongitude(-190);
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(170, 1.e-10));
    }

    SECTION("Whitespace is removed from the authority")
    {
        origin.setAuthority(" uu ");
        REQUIRE(origin.getAuthority() == "UU");
        REQUIRE_THROWS_AS(origin.setAuthority("   "), std::invalid_argument);
    }

    SECTION("Bogus flag")
    {
        origin.setBogus();
        REQUIRE(origin.isBogus());
        origin.unsetBogus();
        REQUIRE_FALSE(origin.isBogus());
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(origin.setAuthority(""), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setLatitude(91), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setLatitude(-91), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(-11), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDepth(1001), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setAlgorithm("ABCDEFGHIJKLMNOP"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setSubSource("123456789"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setGap(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setGap(361), std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setDistanceToNearestStation(-1),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(origin.setWeightedRootMeanSquaredError(-1),
                          std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        origin.setLatitude(40.5);
        origin.setBogus();

        Origin copy{origin};
        REQUIRE_THAT(copy.getLatitude(), WithinAbs(40.5, 1.e-12));
        REQUIRE(copy.isBogus());

        Origin moved{std::move(copy)};
        REQUIRE(moved.haveLatitude());

        origin.clear();
        REQUIRE_FALSE(origin.haveLatitude());
        REQUIRE_FALSE(origin.isBogus());
        REQUIRE(moved.haveLatitude());
    }
}
