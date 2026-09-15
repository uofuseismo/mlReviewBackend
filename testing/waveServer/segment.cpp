#include <array>
#include <chrono>
#include <cstdint>
#include <vector>
#include "mlReview/waveServer/segment.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace MLReview::WaveServer;
using Catch::Matchers::WithinAbs;

TEST_CASE("MLReview::WaveServer::Segment", "[waveServer][segment]")
{
    Segment segment;

    SECTION("Defaults")
    {
        REQUIRE(segment.getStartTime() == std::chrono::microseconds {0});
        REQUIRE_FALSE(segment.haveSamplingRate());
        REQUIRE_THROWS_AS(segment.getSamplingRate(), std::runtime_error);
        REQUIRE_THROWS_AS(segment.getEndTime(), std::runtime_error);
        REQUIRE(segment.getDataType() == Segment::DataType::Undefined);
        REQUIRE(segment.getNumberOfSamples() == 0);
        std::vector<double> data;
        REQUIRE_THROWS_AS(segment.getData(&data), std::runtime_error);
    }

    SECTION("Start time, sampling rate, and end time")
    {
        segment.setStartTime(10.0);
        segment.setSamplingRate(100);
        segment.setData(std::vector<double> (101, 1.0));

        REQUIRE(segment.getStartTime() == std::chrono::microseconds {10000000});
        REQUIRE_THAT(segment.getSamplingRate(), WithinAbs(100, 1.e-12));
        REQUIRE(segment.getNumberOfSamples() == 101);
        REQUIRE(segment.getDataType() == Segment::DataType::Double);
        REQUIRE(segment.getEndTime() == std::chrono::microseconds {11000000});
    }

    SECTION("End time updates regardless of the order properties are set")
    {
        segment.setData(std::vector<int> (11, 0));
        segment.setSamplingRate(10);
        segment.setStartTime(std::chrono::microseconds {5000000});
        REQUIRE(segment.getEndTime() == std::chrono::microseconds {6000000});
    }

    SECTION("Data types")
    {
        segment.setData(std::vector<int> {1, 2, 3});
        REQUIRE(segment.getDataType() == Segment::DataType::Integer32);
        REQUIRE(segment.getNumberOfSamples() == 3);

        segment.setData(std::vector<float> {1, 2, 3, 4});
        REQUIRE(segment.getDataType() == Segment::DataType::Float);
        REQUIRE(segment.getNumberOfSamples() == 4);

        segment.setData(std::vector<int64_t> {1, 2});
        REQUIRE(segment.getDataType() == Segment::DataType::Integer64);
        REQUIRE(segment.getNumberOfSamples() == 2);

        const std::vector<double> doubles{1.5, 2.5};
        segment.setData(doubles);
        REQUIRE(segment.getDataType() == Segment::DataType::Double);
        std::vector<double> result;
        segment.getData(&result);
        REQUIRE(result == doubles);
    }

    SECTION("Get data converts to the requested type")
    {
        segment.setData(std::vector<int> {-1, 0, 7});
        std::vector<double> asDouble;
        segment.getData(&asDouble);
        REQUIRE(asDouble == std::vector<double> {-1, 0, 7});
        std::vector<int64_t> asInt64;
        segment.getData(&asInt64);
        REQUIRE(asInt64 == std::vector<int64_t> {-1, 0, 7});
    }

    SECTION("Set data from a pointer")
    {
        const std::array<int, 3> values{4, 5, 6};
        segment.setData(static_cast<const void *> (values.data()),
                        static_cast<int> (values.size()),
                        Segment::DataType::Integer32);
        REQUIRE(segment.getDataType() == Segment::DataType::Integer32);
        std::vector<int> result;
        segment.getData(&result);
        REQUIRE(result == std::vector<int> {4, 5, 6});
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(segment.setSamplingRate(0), std::invalid_argument);
        REQUIRE_THROWS_AS(segment.setSamplingRate(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(segment.setData(static_cast<const void *> (nullptr),
                                          1, Segment::DataType::Double),
                          std::invalid_argument);
        const std::array<double, 1> value{1};
        REQUIRE_THROWS_AS(segment.setData(static_cast<const void *> (value.data()),
                                          1, Segment::DataType::Undefined),
                          std::invalid_argument);
        std::vector<double> *nullData{nullptr};
        segment.setData(std::vector<double> {1});
        REQUIRE_THROWS_AS(segment.getData(nullData), std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        segment.setSamplingRate(40);
        segment.setData(std::vector<float> {1, 2});

        Segment copy{segment};
        REQUIRE(copy.haveSamplingRate());
        REQUIRE(copy.getNumberOfSamples() == 2);

        Segment moved{std::move(copy)};
        REQUIRE(moved.getNumberOfSamples() == 2);

        segment.clear();
        REQUIRE_FALSE(segment.haveSamplingRate());
        REQUIRE(segment.getNumberOfSamples() == 0);
        REQUIRE(moved.haveSamplingRate());
    }
}
