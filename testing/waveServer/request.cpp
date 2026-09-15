#include <chrono>
#include <string>
#include "mlReview/waveServer/request.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace MLReview::WaveServer;

TEST_CASE("MLReview::WaveServer::Request", "[waveServer][request]")
{
    Request request;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(request.haveNetwork());
        REQUIRE_FALSE(request.haveStation());
        REQUIRE_FALSE(request.haveChannel());
        REQUIRE_FALSE(request.haveLocationCode());
        REQUIRE_FALSE(request.haveStartAndEndTime());
        REQUIRE_THROWS_AS(request.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(request.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(request.getChannel(), std::runtime_error);
        REQUIRE_THROWS_AS(request.getLocationCode(), std::runtime_error);
        REQUIRE_THROWS_AS(request.getStartTime(), std::runtime_error);
        REQUIRE_THROWS_AS(request.getEndTime(), std::runtime_error);
    }

    SECTION("Setters and getters upper-case the codes")
    {
        request.setNetwork("uu");
        request.setStation("ctu");
        request.setChannel("hhz");
        request.setLocationCode("01");
        request.setStartAndEndTime(std::pair<double, double> {10.0, 20.5});

        REQUIRE(request.getNetwork() == "UU");
        REQUIRE(request.getStation() == "CTU");
        REQUIRE(request.getChannel() == "HHZ");
        REQUIRE(request.getLocationCode() == "01");
        REQUIRE(request.getStartTime() == std::chrono::microseconds {10000000});
        REQUIRE(request.getEndTime() == std::chrono::microseconds {20500000});
    }

    SECTION("Whitespace is removed")
    {
        request.setNetwork(" uu ");
        request.setStation("c tu");
        request.setChannel(" hhz");
        request.setLocationCode("01 ");
        REQUIRE(request.getNetwork() == "UU");
        REQUIRE(request.getStation() == "CTU");
        REQUIRE(request.getChannel() == "HHZ");
        REQUIRE(request.getLocationCode() == "01");
        REQUIRE_THROWS_AS(request.setChannel("   "), std::invalid_argument);
    }

    SECTION("Start and end time in microseconds")
    {
        request.setStartAndEndTime(
            std::pair<std::chrono::microseconds, std::chrono::microseconds>
            {std::chrono::microseconds {1}, std::chrono::microseconds {2}});
        REQUIRE(request.getStartTime() == std::chrono::microseconds {1});
        REQUIRE(request.getEndTime() == std::chrono::microseconds {2});
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(request.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(request.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(request.setChannel(""), std::invalid_argument);
        REQUIRE_THROWS_AS(request.setLocationCode(""), std::invalid_argument);
        REQUIRE_THROWS_AS(
            request.setStartAndEndTime(std::pair<double, double> {20.0, 10.0}),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            request.setStartAndEndTime(std::pair<double, double> {10.0, 10.0}),
            std::invalid_argument);
        REQUIRE_FALSE(request.haveStartAndEndTime());
    }

    SECTION("Equality")
    {
        REQUIRE(Request {} == Request {});

        request.setNetwork("UU");
        request.setStation("CTU");
        request.setChannel("HHZ");
        request.setLocationCode("01");
        request.setStartAndEndTime(std::pair<double, double> {10.0, 20.0});

        Request copy{request};
        REQUIRE(copy == request);
        REQUIRE_FALSE(copy != request);

        copy.setChannel("HHN");
        REQUIRE(copy != request);

        Request partial;
        partial.setNetwork("UU");
        REQUIRE(partial != request);
    }

    SECTION("Move and clear")
    {
        request.setNetwork("UU");

        Request moved{std::move(request)};
        REQUIRE(moved.getNetwork() == "UU");

        moved.clear();
        REQUIRE_FALSE(moved.haveNetwork());
    }
}
