#include <string>
#include "mlReview/database/aqms/assocaro.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::AQMS;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::Database::AQMS::AssocArO", "[database][aqms][assocaro]")
{
    AssocArO assocaro;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(assocaro.haveAuthority());
        REQUIRE_FALSE(assocaro.haveOriginIdentifier());
        REQUIRE_FALSE(assocaro.haveArrivalIdentifier());
        REQUIRE_THROWS_AS(assocaro.getAuthority(), std::runtime_error);
        REQUIRE_THROWS_AS(assocaro.getOriginIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(assocaro.getArrivalIdentifier(), std::runtime_error);
        REQUIRE_FALSE(assocaro.getSubSource().has_value());
        REQUIRE_FALSE(assocaro.getPhase().has_value());
        REQUIRE_FALSE(assocaro.getSourceReceiverDistance().has_value());
        REQUIRE_FALSE(assocaro.getSourceToReceiverAzimuth().has_value());
        REQUIRE_FALSE(assocaro.getInputWeight().has_value());
        REQUIRE_FALSE(assocaro.getTravelTimeResidual().has_value());
        REQUIRE_FALSE(assocaro.getTakeOffAngle().has_value());
        REQUIRE_FALSE(assocaro.getReviewFlag().has_value());
    }

    SECTION("Setters and getters")
    {
        assocaro.setAuthority("uu");
        assocaro.setOriginIdentifier(10);
        assocaro.setArrivalIdentifier(20);
        assocaro.setSubSource("Jiggle");
        assocaro.setPhase("S");
        assocaro.setSourceReceiverDistance(12.5);
        assocaro.setSourceToReceiverAzimuth(45);
        assocaro.setInputWeight(0.5);
        assocaro.setTravelTimeResidual(-0.1);
        assocaro.setTakeOffAngle(95);
        assocaro.setReviewFlag(AssocArO::ReviewFlag::Automatic);

        REQUIRE(assocaro.getAuthority() == "UU");
        REQUIRE(assocaro.getOriginIdentifier() == 10);
        REQUIRE(assocaro.getArrivalIdentifier() == 20);
        REQUIRE(*assocaro.getSubSource() == "Jiggle");
        REQUIRE(*assocaro.getPhase() == "S");
        REQUIRE_THAT(*assocaro.getSourceReceiverDistance(),
                     WithinAbs(12.5, 1.e-12));
        REQUIRE_THAT(*assocaro.getSourceToReceiverAzimuth(),
                     WithinAbs(45, 1.e-12));
        REQUIRE_THAT(*assocaro.getInputWeight(), WithinAbs(0.5, 1.e-12));
        REQUIRE_THAT(*assocaro.getTravelTimeResidual(),
                     WithinAbs(-0.1, 1.e-12));
        REQUIRE_THAT(*assocaro.getTakeOffAngle(), WithinAbs(95, 1.e-12));
        REQUIRE(*assocaro.getReviewFlag() == AssocArO::ReviewFlag::Automatic);
    }

    SECTION("Whitespace is removed")
    {
        assocaro.setAuthority(" uu ");
        assocaro.setPhase(" S ");
        REQUIRE(assocaro.getAuthority() == "UU");
        REQUIRE(*assocaro.getPhase() == "S");
        REQUIRE_THROWS_AS(assocaro.setAuthority("  "), std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setPhase("  "), std::invalid_argument);
    }

    SECTION("Boundary values")
    {
        assocaro.setSourceReceiverDistance(0);
        REQUIRE(assocaro.getSourceReceiverDistance().has_value());
        assocaro.setSourceToReceiverAzimuth(0);
        REQUIRE(assocaro.getSourceToReceiverAzimuth().has_value());
        assocaro.setInputWeight(0);
        REQUIRE(assocaro.getInputWeight().has_value());
        assocaro.setTakeOffAngle(0);
        REQUIRE_THAT(*assocaro.getTakeOffAngle(), WithinAbs(0, 1.e-12));
        assocaro.setTakeOffAngle(180);
        REQUIRE_THAT(*assocaro.getTakeOffAngle(), WithinAbs(180, 1.e-12));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(assocaro.setAuthority(""), std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setAuthority("ABCDEFGHIJKLMNOP"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setSubSource("123456789"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setPhase(""), std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setSourceReceiverDistance(-1),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setSourceToReceiverAzimuth(-1),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setSourceToReceiverAzimuth(360),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setInputWeight(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setTakeOffAngle(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(assocaro.setTakeOffAngle(181),
                          std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        assocaro.setOriginIdentifier(10);
        assocaro.setPhase("P");

        AssocArO copy{assocaro};
        REQUIRE(copy.getOriginIdentifier() == 10);
        REQUIRE(*copy.getPhase() == "P");

        AssocArO moved{std::move(copy)};
        REQUIRE(moved.getOriginIdentifier() == 10);

        assocaro.clear();
        REQUIRE_FALSE(assocaro.haveOriginIdentifier());
        REQUIRE_FALSE(assocaro.getPhase().has_value());
        REQUIRE(moved.haveOriginIdentifier());
    }
}
