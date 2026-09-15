#include <string>
#include "mlReview/database/aqms/event.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace MLReview::Database::AQMS;

TEST_CASE("MLReview::Database::AQMS::Event", "[database][aqms][event]")
{
    Event event;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_FALSE(event.haveAuthority());
        REQUIRE_FALSE(event.haveVersion());
        REQUIRE_THROWS_AS(event.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(event.getAuthority(), std::runtime_error);
        REQUIRE_THROWS_AS(event.getVersion(), std::runtime_error);
        REQUIRE_FALSE(event.getPreferredOriginIdentifier().has_value());
        REQUIRE_FALSE(event.getPreferredMagnitudeIdentifier().has_value());
        REQUIRE_FALSE(event.getPreferredMechanismIdentifier().has_value());
        REQUIRE_FALSE(event.getCommentIdentifier().has_value());
        REQUIRE_FALSE(event.getSubSource().has_value());
        REQUIRE_FALSE(event.getType().has_value());
        REQUIRE_FALSE(event.getSelectFlag());
    }

    SECTION("Setters and getters")
    {
        event.setIdentifier(100);
        event.setAuthority("uu");
        event.setVersion(3);
        event.setPreferredOriginIdentifier(1);
        event.setPreferredMagnitudeIdentifier(2);
        event.setPreferredMechanismIdentifier(3);
        event.setCommentIdentifier(4);
        event.setSubSource("Jiggle");
        event.setType(Event::Type::QuarryBlast);

        REQUIRE(event.getIdentifier() == 100);
        REQUIRE(event.getAuthority() == "UU");
        REQUIRE(event.getVersion() == 3);
        REQUIRE(*event.getPreferredOriginIdentifier() == 1);
        REQUIRE(*event.getPreferredMagnitudeIdentifier() == 2);
        REQUIRE(*event.getPreferredMechanismIdentifier() == 3);
        REQUIRE(*event.getCommentIdentifier() == 4);
        REQUIRE(*event.getSubSource() == "Jiggle");
        REQUIRE(*event.getType() == Event::Type::QuarryBlast);
    }

    SECTION("Whitespace is removed from the authority")
    {
        event.setAuthority(" u u ");
        REQUIRE(event.getAuthority() == "UU");
        REQUIRE_THROWS_AS(event.setAuthority("   "), std::invalid_argument);
    }

    SECTION("Select flag")
    {
        event.setSelectFlag();
        REQUIRE(event.getSelectFlag());
        event.unsetSelectFlag();
        REQUIRE_FALSE(event.getSelectFlag());
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(event.setAuthority(""), std::invalid_argument);
        REQUIRE_THROWS_AS(event.setAuthority("ABCDEFGHIJKLMNOP"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(event.setSubSource("123456789"),
                          std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        event.setIdentifier(100);
        event.setType(Event::Type::Earthquake);

        Event copy{event};
        REQUIRE(copy.getIdentifier() == 100);
        REQUIRE(*copy.getType() == Event::Type::Earthquake);

        Event moved{std::move(copy)};
        REQUIRE(moved.getIdentifier() == 100);

        event.clear();
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_FALSE(event.getType().has_value());
        REQUIRE(moved.haveIdentifier());
    }
}
