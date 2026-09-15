#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "mlReview/service/catalog/event.hpp"
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::Service::Catalog;
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

TEST_CASE("MLReview::Service::Catalog::Event", "[service][catalog][event]")
{
    Event event;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_FALSE(event.havePreferredOrigin());
        REQUIRE_THROWS_AS(event.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(event.getPreferredOrigin(), std::runtime_error);
        REQUIRE_FALSE(event.getAQMSEventIdentifiers().has_value());
        REQUIRE_FALSE(event.wasReviewed().has_value());
        REQUIRE_FALSE(event.wasSubmittedToCloudCatalog().has_value());
    }

    SECTION("Setters and getters")
    {
        event.setIdentifier(60000001);
        event.setPreferredOrigin(makeOrigin());
        event.toggleReviewed(true);
        event.toggleSubmittedToCloudCatalog(false);

        REQUIRE(event.getIdentifier() == 60000001);
        REQUIRE(event.havePreferredOrigin());
        REQUIRE_THAT(event.getPreferredOrigin().getLatitude(),
                     WithinAbs(40.5, 1.e-12));
        REQUIRE(*event.wasReviewed());
        REQUIRE_FALSE(*event.wasSubmittedToCloudCatalog());
    }

    SECTION("AQMS identifiers are sorted and de-duplicated")
    {
        event.setAQMSEventIdentifiers({3, 1, 3, 2});
        auto identifiers = event.getAQMSEventIdentifiers();
        REQUIRE(identifiers.has_value());
        REQUIRE(*identifiers == std::vector<int64_t> {1, 2, 3});

        event.setAQMSEventIdentifiers({});
        REQUIRE_FALSE(event.getAQMSEventIdentifiers().has_value());
    }

    SECTION("Preferred origin must be complete")
    {
        Origin noLongitude;
        noLongitude.setTime(1700000000.0);
        noLongitude.setLatitude(40.5);
        noLongitude.setDepth(8000);
        REQUIRE_THROWS_AS(event.setPreferredOrigin(noLongitude),
                          std::invalid_argument);
        REQUIRE_FALSE(event.havePreferredOrigin());
    }

    SECTION("Construct from JSON")
    {
        const auto object = nlohmann::json::parse(R"""(
{
  "eventIdentifier": 60000002,
  "aqmsEventIdentifiers": [80000002],
  "submittedToCloudCatalog": true,
  "parametricData": {
    "preferredOrigin": {
      "time": 1700000000.5,
      "latitude": 44.5,
      "longitude": -110.7,
      "depth": 5000,
      "reviewStatus": "human",
      "arrivals": [
        {"network": "WY", "station": "YHB", "channel1": "HHZ",
         "channel2": "HHN", "channel3": "HHE", "locationCode": "01",
         "phase": "P", "time": 1700000002.0, "residual": 0.1}
      ]
    }
  }
})""");
        Event fromJSON{object};
        REQUIRE(fromJSON.getIdentifier() == 60000002);
        REQUIRE(*fromJSON.getAQMSEventIdentifiers() ==
                std::vector<int64_t> {80000002});
        REQUIRE(*fromJSON.wasSubmittedToCloudCatalog());
        REQUIRE(*fromJSON.wasReviewed());
        auto origin = fromJSON.getPreferredOrigin();
        REQUIRE_THAT(origin.getLatitude(), WithinAbs(44.5, 1.e-12));
        REQUIRE_THAT(origin.getLongitude(), WithinAbs(-110.7, 1.e-12));
        const auto &arrivals = origin.getArrivalsReference();
        REQUIRE(arrivals.size() == 1);
        REQUIRE(arrivals.at(0).getStation() == "YHB");
    }

    SECTION("Copy, move, and clear")
    {
        event.setIdentifier(7);
        event.toggleReviewed(false);

        Event copy{event};
        REQUIRE(copy.getIdentifier() == 7);
        REQUIRE(copy.wasReviewed().has_value());

        Event moved{std::move(copy)};
        REQUIRE(moved.getIdentifier() == 7);

        event.clear();
        REQUIRE_FALSE(event.haveIdentifier());
        REQUIRE_FALSE(event.wasReviewed().has_value());
        REQUIRE(moved.haveIdentifier());
    }
}
