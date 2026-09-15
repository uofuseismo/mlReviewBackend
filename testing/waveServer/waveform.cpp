#include <chrono>
#include <string>
#include <vector>
#include "mlReview/waveServer/segment.hpp"
#include "mlReview/waveServer/waveform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace MLReview::WaveServer;

namespace
{
Segment makeSegment(const double startTime, const int nSamples,
                    const double samplingRate = 10)
{
    Segment segment;
    segment.setStartTime(startTime);
    segment.setSamplingRate(samplingRate);
    segment.setData(std::vector<double> (nSamples, 1.0));
    return segment;
}
}

TEST_CASE("MLReview::WaveServer::Waveform", "[waveServer][waveform]")
{
    Waveform waveform;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(waveform.haveNetwork());
        REQUIRE_FALSE(waveform.haveStation());
        REQUIRE_FALSE(waveform.haveChannel());
        REQUIRE_FALSE(waveform.haveLocationCode());
        REQUIRE_THROWS_AS(waveform.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(waveform.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(waveform.getChannel(), std::runtime_error);
        REQUIRE_THROWS_AS(waveform.getLocationCode(), std::runtime_error);
        REQUIRE(waveform.getNumberOfSegments() == 0);
        REQUIRE(waveform.begin() == waveform.end());
    }

    SECTION("Setters and getters upper-case the codes")
    {
        waveform.setNetwork("uu");
        waveform.setStation("ctu");
        waveform.setChannel("hhz");
        waveform.setLocationCode("01");

        REQUIRE(waveform.getNetwork() == "UU");
        REQUIRE(waveform.getStation() == "CTU");
        REQUIRE(waveform.getChannel() == "HHZ");
        REQUIRE(waveform.getLocationCode() == "01");
    }

    SECTION("Whitespace is removed")
    {
        waveform.setNetwork(" uu ");
        waveform.setStation("c tu");
        waveform.setChannel(" hhz");
        REQUIRE(waveform.getNetwork() == "UU");
        REQUIRE(waveform.getStation() == "CTU");
        REQUIRE(waveform.getChannel() == "HHZ");
        REQUIRE_THROWS_AS(waveform.setNetwork("   "), std::invalid_argument);
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(waveform.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(waveform.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(waveform.setChannel(""), std::invalid_argument);

        Segment noSamplingRate;
        noSamplingRate.setData(std::vector<double> {1, 2});
        REQUIRE_THROWS_AS(waveform.addSegment(std::move(noSamplingRate)),
                          std::invalid_argument);

        Segment noData;
        noData.setSamplingRate(10);
        REQUIRE_THROWS_AS(waveform.addSegment(std::move(noData)),
                          std::invalid_argument);
        REQUIRE(waveform.getNumberOfSegments() == 0);
    }

    SECTION("Segments are kept in time order")
    {
        waveform.addSegment(makeSegment(10.0, 5));
        waveform.addSegment(makeSegment(0.0, 5));
        REQUIRE(waveform.getNumberOfSegments() == 2);
        REQUIRE(waveform.at(0).getStartTime() == std::chrono::microseconds {0});
        REQUIRE(waveform.at(1).getStartTime() ==
                std::chrono::microseconds {10000000});
        REQUIRE_THROWS_AS(waveform.at(2), std::out_of_range);

        int count{0};
        for (const auto &segment : waveform)
        {
            REQUIRE(segment.getNumberOfSamples() == 5);
            count = count + 1;
        }
        REQUIRE(count == 2);
    }

    SECTION("Contiguous segments are merged")
    {
        // 10 samples at 10 Hz starting at 0 ends at 0.9 s so the next
        // sample is expected at 1.0 s.
        waveform.addSegment(makeSegment(0.0, 10));
        waveform.addSegment(makeSegment(1.0, 5));
        waveform.mergeSegments();
        REQUIRE(waveform.getNumberOfSegments() == 1);
        REQUIRE(waveform.at(0).getNumberOfSamples() == 15);
        REQUIRE(waveform.at(0).getStartTime() == std::chrono::microseconds {0});
    }

    SECTION("Segments with a gap are not merged")
    {
        waveform.addSegment(makeSegment(0.0, 10));
        waveform.addSegment(makeSegment(5.0, 5));
        waveform.mergeSegments();
        REQUIRE(waveform.getNumberOfSegments() == 2);
    }

    SECTION("Segments with different sampling rates are not merged")
    {
        waveform.addSegment(makeSegment(0.0, 10, 10));
        waveform.addSegment(makeSegment(1.0, 5, 100));
        waveform.mergeSegments();
        REQUIRE(waveform.getNumberOfSegments() == 2);
    }

    SECTION("Negative sampling period factor")
    {
        waveform.addSegment(makeSegment(0.0, 10));
        waveform.addSegment(makeSegment(1.0, 5));
        REQUIRE_THROWS_AS(waveform.mergeSegments(-1), std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        waveform.setNetwork("UU");
        waveform.addSegment(makeSegment(0.0, 5));

        Waveform copy{waveform};
        REQUIRE(copy.getNetwork() == "UU");
        REQUIRE(copy.getNumberOfSegments() == 1);

        Waveform moved{std::move(copy)};
        REQUIRE(moved.getNumberOfSegments() == 1);

        waveform.clear();
        REQUIRE_FALSE(waveform.haveNetwork());
        REQUIRE(waveform.getNumberOfSegments() == 0);
        REQUIRE(moved.haveNetwork());
    }
}
