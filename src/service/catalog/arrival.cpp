#include <chrono>
#include <cmath>
#include <string>
#include "mlReview/service/catalog/arrival.hpp"
#include "private/isEmpty.hpp"

using namespace MLReview::Service::Catalog;


class Arrival::ArrivalImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mVerticalChannel;
    std::string mNorthChannel;
    std::string mEastChannel;
    std::string mLocationCode;
    std::string mPhase;
    std::chrono::microseconds mTime;
    double mResidual;
    double mDistance{-1};
    double mAzimuth{-1};
    bool mHaveTime{false};
    bool mHavePhase{false};
    bool mHaveIdentifier{false};
    bool mHaveResidual{false};
};

/// Constructor
Arrival::Arrival() :
    pImpl(std::make_unique<ArrivalImpl> ())
{
}

/// Copy constructor
Arrival::Arrival(const Arrival &arrival)
{
    *this = arrival;
}

/// Move constructor
Arrival::Arrival(Arrival &&arrival) noexcept
{
    *this = std::move(arrival);
}

/// Copy assignment
Arrival& Arrival::operator=(const Arrival &arrival)
{
    if (&arrival == this){return *this;}
    pImpl = std::make_unique<ArrivalImpl> (*arrival.pImpl);
    return *this;
}

/// Move assignment
Arrival& Arrival::operator=(Arrival &&arrival) noexcept
{
    if (&arrival == this){return *this;}
    pImpl = std::move(arrival.pImpl);
    return *this;
}

/// Reset class 
void Arrival::clear() noexcept
{
    pImpl = std::make_unique<ArrivalImpl> (); 
}

/// Destructor
Arrival::~Arrival() = default;

/// Network
void Arrival::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Arrival::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool Arrival::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Arrival::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Arrival::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool Arrival::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Arrival::setChannels(const std::string &verticalChannel,
                          const std::string &northChannel,
                          const std::string &eastChannel)
{
    if (::isEmpty(verticalChannel))
    {   
        throw std::invalid_argument("Vertical channel is empty");
    }   
    if (verticalChannel.size() != 3)
    {   
        throw std::invalid_argument("Vertical channel size not 3");
    }   
    if (!::isEmpty(northChannel) && !::isEmpty(eastChannel))
    {   
        if (northChannel.size() != 3)
        {
            throw std::invalid_argument("North channel size not 3");
        }
        if (eastChannel.size() != 3)
        {
            throw std::invalid_argument("East channel size not 3");
        }
        pImpl->mVerticalChannel = verticalChannel;
        pImpl->mNorthChannel = northChannel;
        pImpl->mEastChannel = eastChannel;
    }   
    else
    {   
        pImpl->mVerticalChannel = verticalChannel;
        pImpl->mNorthChannel.clear();
        pImpl->mEastChannel.clear();
    }   
}

std::string Arrival::getVerticalChannel() const
{
    if (!haveChannels()){std::runtime_error("Channels not set");}
    return pImpl->mVerticalChannel;
}

std::optional<std::pair<std::string, std::string>> 
    Arrival::getNonVerticalChannels() const
{
    if (!haveChannels()){std::runtime_error("Channels not set");}
    auto northChannel = pImpl->mNorthChannel;
    auto eastChannel = pImpl->mEastChannel;
    if (!northChannel.empty() && !eastChannel.empty())
    {   
        return std::optional<std::pair<std::string, std::string>>
                  {std::pair {northChannel, eastChannel}};
    }   
    return std::nullopt;
}

bool Arrival::haveChannels() const noexcept
{
   return !pImpl->mVerticalChannel.empty();
}

/// Location code
void Arrival::setLocationCode(const std::string &location)
{
    if (::isEmpty(location))
    {
        throw std::invalid_argument("Location code is empty");
    }   
    pImpl->mLocationCode = location;
}

std::string Arrival::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }   
    return pImpl->mLocationCode;
}

bool Arrival::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Arrival time
void Arrival::setTime(const double time) noexcept
{
    auto pickTime
       = std::chrono::microseconds{static_cast<int64_t> (std::round(time*1.e6))};
    setTime(pickTime);
}

void Arrival::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Arrival::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Arrival::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Phase
void Arrival::setPhase(const std::string &phase)
{
    if (::isEmpty(phase)){throw std::invalid_argument("Phase is empty");} 
    pImpl->mPhase = phase;
    pImpl->mHavePhase = true;
}

std::string Arrival::getPhase() const
{
    if (!havePhase()){throw std::runtime_error("Phase not set");}
    return pImpl->mPhase;
}

bool Arrival::havePhase() const noexcept
{
    return pImpl->mHavePhase;
}

/// Residual
void Arrival::setResidual(const double residual) noexcept
{
    pImpl->mResidual = residual;
    pImpl->mHaveResidual = true;
}

std::optional<double> Arrival::getResidual() const noexcept
{
    return pImpl->mHaveResidual ?
           std::optional<double> (pImpl->mResidual) : std::nullopt;
}

/// Distance
void Arrival::setDistance(const double distance)
{
    if (distance < 0)
    {
        throw std::invalid_argument(
            "Source-receiver distance must be non-negative");
    }
    pImpl->mDistance = distance;
}

std::optional<double> Arrival::getDistance() const noexcept
{
    return (pImpl->mDistance >= 0) ?
           std::optional<double> (pImpl->mDistance) : std::nullopt;
}

/// Azimuth
void Arrival::setAzimuth(const double azimuth)
{
    if (azimuth < 0 || azimuth >= 360)
    {
        throw std::invalid_argument("Azimuth must be in range [0,360)");
    }
    pImpl->mAzimuth = azimuth;
}

std::optional<double> Arrival::getAzimuth() const noexcept
{
    return (pImpl->mAzimuth >= 0) ?
           std::optional<double> (pImpl->mAzimuth) : std::nullopt;
}

/// JSON
nlohmann::json MLReview::Service::Catalog::toObject(const Arrival &arrival)
{
    nlohmann::json result;
    result["network"] = arrival.getNetwork();
    result["station"] = arrival.getStation();
    result["channel1"] = arrival.getVerticalChannel();
    auto nonVerticalChannels = arrival.getNonVerticalChannels();
    if (nonVerticalChannels)
    {
        result["channel2"] = nonVerticalChannels->first;
        result["channel3"] = nonVerticalChannels->second;
    }
    if (arrival.haveLocationCode())
    {
        result["locationCode"] = arrival.getLocationCode();
    }
    result["phase"] = arrival.getPhase();
    result["time"] = arrival.getTime().count()*1.e-6;
    auto residual = arrival.getResidual();
    if (residual){result["residual"] = *residual;}
    auto distance = arrival.getDistance();
    if (distance){result["distance"] = *distance;}
    auto azimuth = arrival.getAzimuth();
    if (azimuth){result["azimuth"] = *azimuth;}
    return result;
}
