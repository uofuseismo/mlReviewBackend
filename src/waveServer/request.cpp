#include <cmath>
#include <algorithm>
#include <chrono>
#include "mlReview/waveServer/request.hpp"

namespace
{
std::string convertString(const std::string &input)
{
    auto result = input;
    std::remove_if(result.begin(), result.end(), ::isspace);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}
}

using namespace MLReview::WaveServer;

class Request::RequestImpl
{
public:
    std::chrono::microseconds mStartTime{0};
    std::chrono::microseconds mEndTime{0};
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    bool mHaveStartAndEndTime{false};
};

/// Constructor
Request::Request() :
    pImpl(std::make_unique<RequestImpl> ())
{
}

/// Copy constructor
Request::Request(const Request &request)
{
    *this = request;
}

/// Move constructor
Request::Request(Request &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
Request& Request::operator=(const Request &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<RequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
Request& Request::operator=(Request &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Resets the class
void Request::clear() noexcept
{
    pImpl = std::make_unique<RequestImpl> ();
}

/// Destructor
Request::~Request() = default;

/// Network
void Request::setNetwork(const std::string &networkIn)
{
    auto network = ::convertString(networkIn);
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Request::getNetwork() const
{
    if (!haveNetwork())
    {
        throw std::runtime_error("Network not set");
    }
    return pImpl->mNetwork;
}

bool Request::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Request::setStation(const std::string &stationIn)
{
    auto station = ::convertString(stationIn);
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Request::getStation() const
{
    if (!haveStation())
    {   
        throw std::runtime_error("Station not set");
    }   
    return pImpl->mStation;
}

bool Request::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Request::setChannel(const std::string &channelIn)
{
    auto channel = ::convertString(channelIn);
    if (channel.empty()){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Request::getChannel() const
{
    if (!haveChannel())
    {   
        throw std::runtime_error("Channel not set");
    }   
    return pImpl->mChannel;
}

bool Request::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Request::setLocationCode(const std::string &locationCodeIn)
{
    auto locationCode = ::convertString(locationCodeIn);
    if (locationCode.empty())
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = locationCode;
}

std::string Request::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set");
    }
    return pImpl->mLocationCode;
}

bool Request::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Start and end time
void Request::setStartAndEndTime(
    const std::pair<double, double> &startAndEndTime)
{
    auto t0 = static_cast<int64_t> (std::round(startAndEndTime.first*1.e6));
    auto t1 = static_cast<int64_t> (std::round(startAndEndTime.second*1.e6));
    std::pair<std::chrono::microseconds, 
              std::chrono::microseconds> startAndEndTimeMuS{t0, t1};
    setStartAndEndTime(startAndEndTimeMuS);
}

void Request::setStartAndEndTime(
    const std::pair<std::chrono::microseconds, std::chrono::microseconds>
    &startAndEndTime)
{
    if (startAndEndTime.first >= startAndEndTime.second)
    {
        throw std::invalid_argument("Start time must be less than end time");
    }
    pImpl->mStartTime = startAndEndTime.first;
    pImpl->mEndTime = startAndEndTime.second;
    pImpl->mHaveStartAndEndTime = true;
}

std::chrono::microseconds Request::getStartTime() const
{
    if (!haveStartAndEndTime())
    {
        throw std::runtime_error("Start time not set");
    }
    return pImpl->mStartTime;
}

std::chrono::microseconds Request::getEndTime() const
{
    if (!haveStartAndEndTime())
    {   
        throw std::runtime_error("End time not set");
    }   
    return pImpl->mEndTime;
}

bool Request::haveStartAndEndTime() const noexcept
{
    return pImpl->mHaveStartAndEndTime;
}

bool MLReview::WaveServer::operator==(const Request &lhs, const Request &rhs)
{
    if (lhs.haveNetwork() && rhs.haveNetwork())
    {
        if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    }
    else
    {
        if (lhs.haveNetwork() != rhs.haveNetwork()){return false;}
    }

    if (lhs.haveStation() && rhs.haveStation())
    {
        if (lhs.getStation() != rhs.getStation()){return false;}
    }
    else
    {
        if (lhs.haveStation() != rhs.haveStation()){return false;}
    }

    if (lhs.haveChannel() && rhs.haveChannel())
    {
        if (lhs.getChannel() != rhs.getChannel()){return false;}
    }
    else
    {
        if (lhs.haveChannel() != rhs.haveChannel()){return false;}
    }

    if (lhs.haveLocationCode() && rhs.haveLocationCode())
    {
        if (lhs.getLocationCode() != rhs.getLocationCode()){return false;}
    }
    else
    {
        if (lhs.haveLocationCode() != rhs.haveLocationCode()){return false;}
    }

    if (lhs.haveStartAndEndTime() && rhs.haveStartAndEndTime())
    {
        if (lhs.getStartTime() != rhs.getStartTime()){return false;}
        if (lhs.getEndTime() != rhs.getEndTime()){return false;}
    }   
    else
    {   
        if (lhs.haveStartAndEndTime() != rhs.haveStartAndEndTime())
        {
            return false;
        }
    }   

    return true;
}

bool MLReview::WaveServer::operator!=(const Request &lhs, const Request &rhs)
{
    return !(lhs == rhs);
}
