#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>
#include <spdlog/spdlog.h>
#include "mlReview/waveServer/segment.hpp"
#include "mlReview/waveServer/waveform.hpp"

namespace
{
std::string convertString(const std::string &input)
{
    auto result = input;
    std::remove_if(result.begin(), result.end(), ::isspace);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

template<typename T>
MLReview::WaveServer::Segment merge(const MLReview::WaveServer::Segment &segmentToAppend,
                                    const MLReview::WaveServer::Segment &newSegment)
{
    auto result = segmentToAppend;
    std::vector<T> data0;
    std::vector<T> data1;
    segmentToAppend.getData(&data0);
    newSegment.getData(&data1);
    data0.insert(data0.end(), data1.begin(), data1.end());
    result.setData(std::move(data0)); //data0.data(), data0.size()); // TODO make a move operator
    return result;
}
}

using namespace MLReview::WaveServer;

class Waveform::WaveformImpl
{
public:
    std::vector<Segment> mSegments;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::chrono::microseconds mStartTime{0};
    std::chrono::microseconds mEndTime{0};
};

/// Constructor
Waveform::Waveform() :
    pImpl(std::make_unique<WaveformImpl> ())
{
}

/// Copy constructor
Waveform::Waveform(const Waveform &waveform)
{
    *this = waveform;
}

/// Move constructor
Waveform::Waveform(Waveform &&waveform) noexcept
{
    *this = std::move(waveform);
}

/// Copy assignment
Waveform& Waveform::operator=(const Waveform &waveform)
{
    if (&waveform == this){return *this;}
    pImpl = std::make_unique<WaveformImpl> (*waveform.pImpl);
    return *this;
}

/// Move assignment
Waveform& Waveform::operator=(Waveform &&waveform) noexcept
{
    if (&waveform == this){return *this;}
    pImpl = std::move(waveform.pImpl);
    return *this;
}

/// Reset class/release memory
void Waveform::clear() noexcept
{
    pImpl = std::make_unique<WaveformImpl> ();
}

/// Destructor
Waveform::~Waveform() = default;

/// Network
void Waveform::setNetwork(const std::string &networkIn)
{
    auto network = ::convertString(networkIn);
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Waveform::getNetwork() const
{
    if (!haveNetwork())
    {   
        throw std::runtime_error("Network not set");
    }   
    return pImpl->mNetwork;
}

bool Waveform::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Waveform::setStation(const std::string &stationIn)
{
    auto station = ::convertString(stationIn);
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Waveform::getStation() const
{
    if (!haveStation())
    {   
        throw std::runtime_error("Station not set");
    }   
    return pImpl->mStation;
}

bool Waveform::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Waveform::setChannel(const std::string &channelIn)
{
    auto channel = ::convertString(channelIn);
    if (channel.empty()){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Waveform::getChannel() const
{
    if (!haveChannel())
    {   
        throw std::runtime_error("Channel not set");
    }   
    return pImpl->mChannel;
}

bool Waveform::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Waveform::setLocationCode(const std::string &locationCodeIn)
{
    auto locationCode = ::convertString(locationCodeIn);
    if (locationCode.empty())
    {   
        pImpl->mLocationCode = "--";
    }   
    pImpl->mLocationCode = locationCode;
}

std::string Waveform::getLocationCode() const
{
    if (!haveLocationCode())
    {   
        throw std::runtime_error("Location code not set");
    }   
    return pImpl->mLocationCode;
}

bool Waveform::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Adds a segment
void Waveform::addSegment(Segment &&segment)
{
    if (!segment.haveSamplingRate())
    {
        throw std::invalid_argument("Sampling rate not set on segment");
    }
    if (segment.getNumberOfSamples() < 1)
    {
        throw std::invalid_argument("Segment has no data");
    }
    auto startTime = segment.getStartTime();
    if (startTime > pImpl->mEndTime)
    {
        pImpl->mSegments.push_back(std::move(segment));
    }
    else
    {
        pImpl->mSegments.push_back(std::move(segment));
        std::sort(pImpl->mSegments.begin(), pImpl->mSegments.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                     return lhs.getStartTime() < rhs.getStartTime();
                  });
    }
    pImpl->mStartTime = pImpl->mSegments.front().getStartTime();
    pImpl->mEndTime = pImpl->mSegments.back().getEndTime();
}

/// Number of segments
int Waveform::getNumberOfSegments() const noexcept
{
    return static_cast<int> (pImpl->mSegments.size());
}
 
/// Merges segments
void Waveform::mergeSegments(
    const double samplingPeriodFactor)
{
    auto nSegments = getNumberOfSegments();
    if (nSegments < 2){return;} // Nothign to do
    if (samplingPeriodFactor < 0)
    {
        throw std::invalid_argument(
            "Sampling period factor must be non-negative");
    }
    auto startTimePreMerge = pImpl->mStartTime;
    auto endTimePreMerge = pImpl->mEndTime;
    std::vector<Segment> mergedSegments;
    // Get total number of samples
    int nSamplesPreMerge{0};
    for (const auto &segment : pImpl->mSegments)
    {
        nSamplesPreMerge = nSamplesPreMerge + segment.getNumberOfSamples();
    }
    // First one is always coming over
    mergedSegments.push_back(pImpl->mSegments.at(0));
    // This is a look behind algorithm.  But we compare with the merged
    // segments to deal with round-off error accumulation.
    for (int iSegment = 1; iSegment < nSegments; ++iSegment)
    {
        auto segment0 = mergedSegments.back();
        auto endTime0 = segment0.getEndTime().count()*1.e-6;
        auto startTime1
            = pImpl->mSegments[iSegment].getStartTime().count()*1.e-6;
        auto samplingPeriod0 = 1./segment0.getSamplingRate();
        auto samplingPeriod1
            = 1./pImpl->mSegments[iSegment].getSamplingRate();
        auto dataType0 = segment0.getDataType();
        auto dataType1 = pImpl->mSegments[iSegment].getDataType();
        // Sampling periods match to 5000 Hz so this is fine
        bool merge{false};
        if (dataType0 == dataType1)
        {
            if (std::abs(samplingPeriod0 - samplingPeriod1) < 0.0002)
            {
                auto deltaTime = startTime1 - (endTime0 + samplingPeriod0);
                if (deltaTime < samplingPeriod0*samplingPeriodFactor)
                {
                    merge = true;
                }
            }
        }
        // Merge?  This is effectively appending data
        if (merge)
        {
            spdlog::debug("Merging packets");
            if (dataType0 == Segment::DataType::Integer32)
            {
                mergedSegments.back()
                    = ::merge<int>(segment0, pImpl->mSegments[iSegment]);
            }
            else if (dataType0 == Segment::DataType::Float)
            {
                mergedSegments.back()
                    = ::merge<float>(segment0, pImpl->mSegments[iSegment]);
            }
            else if (dataType0 == Segment::DataType::Double)
            {   
                mergedSegments.back()
                    = ::merge<double>(segment0, pImpl->mSegments[iSegment]);
            }
            else if (dataType0 == Segment::DataType::Integer64)
            {   
                mergedSegments.back()
                    = ::merge<int64_t>(segment0, pImpl->mSegments[iSegment]);
            }
            else
            {
                spdlog::warn("Unhandled data type; pushing back packet"); 
                mergedSegments.push_back(pImpl->mSegments[iSegment]);
            }
        }
        else
        {
            spdlog::debug("Gap detected; not merging");
            mergedSegments.push_back(pImpl->mSegments[iSegment]);
        }
    }
    // Sort and update
    std::sort(mergedSegments.begin(), mergedSegments.end(),
              [](const auto &lhs, const auto &rhs)
              {
                 return lhs.getStartTime() < rhs.getStartTime();
              });
    auto startTimePostMerge = mergedSegments.front().getStartTime();
    auto endTimePostMerge = mergedSegments.front().getEndTime(); 
    int nSamplesPostMerge{0};
    for (const auto &segment : mergedSegments)
    {
        nSamplesPostMerge = nSamplesPostMerge + segment.getNumberOfSamples();
    }
    bool mergeFailed{false};
    if (startTimePostMerge != startTimePreMerge){mergeFailed = true;} 
    if (nSamplesPreMerge != nSamplesPostMerge){mergeFailed = true;}
    // TODO can probably add something like (endTimePostMerge - endTimePreMerge) < tol
//std::cout << startTimePreMerge << " " << startTimePostMerge << std::endl;
//std::cout << endTimePreMerge << " " << endTimePostMerge << std::endl;
    if (!mergeFailed)
    {
        pImpl->mSegments = std::move(mergedSegments);
        pImpl->mStartTime = startTimePostMerge;
        pImpl->mEndTime = endTimePostMerge;
    }
    else
    {
        spdlog::warn("Merge failed; will not update");
    }
}

/// Iterators
Waveform::iterator Waveform::begin()
{
    return pImpl->mSegments.begin();
}

Waveform::const_iterator Waveform::begin() const noexcept
{
    return pImpl->mSegments.begin();
}

Waveform::const_iterator Waveform::cbegin() const noexcept
{
    return pImpl->mSegments.cbegin();
}

Waveform::iterator Waveform::end()
{
    return pImpl->mSegments.end();
}

Waveform::const_iterator Waveform::end() const noexcept
{
    return pImpl->mSegments.cend();
}

Waveform::const_iterator Waveform::cend() const noexcept
{
    return pImpl->mSegments.cend();
}

/// To object
nlohmann::json MLReview::WaveServer::toObject(const Waveform &waveform)
{
    if (!waveform.haveNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!waveform.haveStation())
    {   
        throw std::invalid_argument("Station not set");
    }
    if (!waveform.haveChannel())
    {   
        throw std::invalid_argument("Channel not set");
    }
    nlohmann::json result;
    result["network"] = waveform.getNetwork();
    result["station"] = waveform.getStation();
    result["channel"] = waveform.getChannel();
    if (waveform.haveLocationCode())
    {
        result["locationCode"] = waveform.getLocationCode();
    }
    else
    {
        result["locationCode"] = nullptr;
    }
    if (waveform.getNumberOfSegments() > 0)
    {
        nlohmann::json segments;
        for (const auto &segment : waveform)
        {
            auto segmentObject = toObject(segment);
            segments.push_back(std::move(segmentObject)); 
        }
        result["segments"] = segments;
    }
    else
    {
        result["segments"] = nullptr;
    }
    return result;
}

const Segment& Waveform::at(const size_t index) const
{
    return pImpl->mSegments.at(index);
}

