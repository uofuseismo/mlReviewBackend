#include <string>
#include <chrono>
#include "mlReview/service/stations/station.hpp"
#include "private/lonTo180.hpp"

using namespace MLReview::Service::Stations;

class Station::StationImpl
{
public:
    std::chrono::seconds mOnDate;
    std::chrono::seconds mOffDate;
    std::string mNetwork;
    std::string mName;
    std::string mDescription;
    double mLatitude;
    double mLongitude;
    double mElevation;
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveElevation{false};
    bool mHaveOnOffDate{false};
};

/// Constructor
Station::Station() :
    pImpl(std::make_unique<StationImpl> ())
{
}

/// Copy constructor
Station::Station(const Station &station)
{
    *this = station;
}

/// Move constructor
Station::Station(Station &&station) noexcept
{
    *this = std::move(station);
}

/// Copy assignment
Station& Station::operator=(const Station &station)
{
    if (&station == this){return *this;}
    pImpl = std::make_unique<StationImpl> (*station.pImpl);
    return *this;
}

/// Move assignment
Station& Station::operator=(Station &&station) noexcept
{
    if (&station == this){return *this;}
    pImpl = std::move(station.pImpl);
    return *this;
}

/// Reset class
void Station::clear() noexcept
{
    pImpl = std::make_unique<StationImpl> ();
}

/// Destructor
Station::~Station() = default;

/// Network
void Station::setNetwork(const std::string &network)
{
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Station::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool Station::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Name
void Station::setName(const std::string &station)
{
    if (station.empty()){throw std::invalid_argument("Station name is empty");}
    pImpl->mName = station;
}

std::string Station::getName() const
{
    if (!haveName()){throw std::runtime_error("Name not set");}
    return pImpl->mName;
}

bool Station::haveName() const noexcept
{
    return !pImpl->mName.empty();
}

/// Description
void Station::setDescription(const std::string &description) noexcept
{
    pImpl->mDescription = description;
}

std::string Station::getDescription() const noexcept
{
    return pImpl->mDescription;
}

/// Latitude
void Station::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90) 
    {   
        throw std::invalid_argument("Latitude must be in [-90,90]");
    }   
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double Station::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool Station::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void Station::setLongitude(const double lonIn) noexcept
{
    pImpl->mLongitude = ::lonTo180(lonIn);
    pImpl->mHaveLongitude = true;
}

double Station::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool Station::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}

/// Elevation
void Station::setElevation(const double elevation)
{
    if (elevation > 8600 || elevation < -10000)
    {
        throw std::invalid_argument(
            "Station elevation must be in range [-8600, -10000]");
    }
    pImpl->mElevation = elevation;
    pImpl->mHaveElevation = true;
}

double Station::getElevation() const
{
    if (!haveElevation()){throw std::runtime_error("Elevation not set");}
    return pImpl->mElevation;
}

bool Station::haveElevation() const noexcept
{
    return pImpl->mHaveElevation;
}

/// On/off date
void Station::setOnOffDate(
    const std::pair<std::chrono::seconds,
                    std::chrono::seconds> &onOffDate)
{
    if (onOffDate.first >= onOffDate.second)
    {   
        throw std::invalid_argument(
            "onOffDate.first must be less than onOffDate.second");
    }   
    pImpl->mOnDate = onOffDate.first;
    pImpl->mOffDate = onOffDate.second;
    pImpl->mHaveOnOffDate = true;
}

std::chrono::seconds Station::getOnDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOnDate;
}

std::chrono::seconds Station::getOffDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOffDate;
}

bool Station::haveOnOffDate() const noexcept
{
    return pImpl->mHaveOnOffDate;
}

/// Is local?
bool Station::isLocal() const
{
    auto network = getNetwork();
    if (network == "UU" || network == "WY")
    {
        return true;
    }
    else if (network == "NP")
    {
        auto name = getName();
        if (name == "7234"){return true;}
    }
    return false;
}

/// To object
nlohmann::json MLReview::Service::Stations::toObject(const Station &station)
{
    nlohmann::json result;
    result["network"] = station.getNetwork();
    result["name"] = station.getName();
    result["description"] = station.getDescription();
    result["latitude"] = station.getLatitude();
    result["longitude"] = station.getLongitude();
    result["elevation"] = station.getElevation();
    result["isLocal"] = station.isLocal();
    result["onDate"] = station.getOnDate().count();
    result["offDate"] = station.getOffDate().count();
    return result;
}
