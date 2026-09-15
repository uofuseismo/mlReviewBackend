#include <chrono>
#include <string>
#include "mlReview/database/machineLearning/event.hpp"
#include "mlReview/database/machineLearning/origin.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Database::MachineLearning::RealTime;
using Catch::Matchers::WithinAbs;

namespace
{
Origin makeOrigin()
{
    Origin origin;
    origin.setTime(1700000000.0);
    origin.setLatitude(40.5);
    origin.setLongitude(-111.9);
    origin.setDepth(8000);
    return origin;
}
}

TEST_CASE("MLReview::Database::MachineLearning::RealTime::Event",
          "[database][machineLearning][event]")
{
    Event event;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_THROWS_AS(event.getIdentifier(), std::runtime_error);
        REQUIRE_FALSE(event.getPreferredOrigin().has_value());
        REQUIRE(event.getType() == Event::Type::Unknown);
        REQUIRE(event.getMonitoringRegion() ==
                Event::MonitoringRegion::Unknown);
        REQUIRE_FALSE(event.getAuthority().has_value());
    }

    SECTION("Setters and getters")
    {
        event.setIdentifier(42);
        event.setType(Event::Type::Earthquake);
        event.setMonitoringRegion(Event::MonitoringRegion::Yellowstone);
        event.setAuthority("UU");
        event.setPreferredOrigin(makeOrigin());

        REQUIRE(event.getIdentifier() == 42);
        REQUIRE(event.getType() == Event::Type::Earthquake);
        REQUIRE(event.getMonitoringRegion() ==
                Event::MonitoringRegion::Yellowstone);
        REQUIRE(*event.getAuthority() == "UU");
        auto preferredOrigin = event.getPreferredOrigin();
        REQUIRE(preferredOrigin.has_value());
        REQUIRE_THAT(preferredOrigin->getLatitude(), WithinAbs(40.5, 1.e-12));
        REQUIRE_THAT(preferredOrigin->getDepth(), WithinAbs(8000, 1.e-12));
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(event.setAuthority(""), std::invalid_argument);

        Origin noTime;
        noTime.setLatitude(40.5);
        noTime.setLongitude(-111.9);
        noTime.setDepth(8000);
        REQUIRE_THROWS_AS(event.setPreferredOrigin(noTime),
                          std::invalid_argument);

        Origin noDepth;
        noDepth.setTime(1700000000.0);
        noDepth.setLatitude(40.5);
        noDepth.setLongitude(-111.9);
        REQUIRE_THROWS_AS(event.setPreferredOrigin(noDepth),
                          std::invalid_argument);
        REQUIRE_FALSE(event.getPreferredOrigin().has_value());
    }

    SECTION("Copy, move, and clear")
    {
        event.setIdentifier(42);
        event.setPreferredOrigin(makeOrigin());

        Event copy{event};
        REQUIRE(copy.getIdentifier() == 42);
        REQUIRE(copy.getPreferredOrigin().has_value());

        Event moved{std::move(copy)};
        REQUIRE(moved.getIdentifier() == 42);

        event.clear();
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_FALSE(event.getPreferredOrigin().has_value());
        REQUIRE(moved.haveIdentifier());
    }
}
