#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"
#include "mlReview/service/catalog/magnitude.hpp"
#include "private/lonTo180.hpp"

using namespace MLReview::Service::Catalog;

namespace
{
std::string eventTypeToString(const Origin::EventType type)
{
    if (type == Origin::EventType::Earthquake)
    {
        return "earthquake";
    }
    else if (type == Origin::EventType::QuarryBlast)
    {
        return "quarryBlast";
    }
    else if (type == Origin::EventType::Unknown)
    {
        return "unknown";
    }
    spdlog::warn("Unhandled event type");
    return "unknown";
}

}

class Origin::OriginImpl
{
public:
    //std::unique_ptr<IMagnitude> mPreferredMagnitude{nullptr};
    std::vector<Arrival> mArrivals;
    std::chrono::microseconds mTime;
    double mLatitude;
    double mLongitude;
    double mDepth;
    int64_t mIdentifier;
    Origin::EventType mEventType{Origin::EventType::Unknown};
    bool mHaveTime{false};
    bool mHaveLongitude{false};
    bool mHaveLatitude{false};
    bool mHaveDepth{false};
    bool mHaveIdentifier{false};
    bool mHavePreferredMagnitude{false};
};

/// Constructor
Origin::Origin() :
    pImpl(std::make_unique<OriginImpl> ()) 
{
}

/// Copy constructor
Origin::Origin(const Origin &origin)
{
    *this = origin;
}

/// Move constructor
Origin::Origin(Origin &&origin) noexcept
{
    *this = std::move(origin);
}

/// Copy assignment
Origin& Origin::operator=(const Origin &origin)
{
    if (&origin == this){return *this;}
    pImpl = std::make_unique<OriginImpl> (*origin.pImpl);
    return *this;
}

/// Move assignment
Origin& Origin::operator=(Origin &&origin) noexcept
{
    if (&origin == this){return *this;}
    pImpl = std::move(origin.pImpl);
    return *this;
}

/// Reset class 
void Origin::clear() noexcept
{
    pImpl = std::make_unique<OriginImpl> ();
}   

/// Destructor
Origin::~Origin() = default;

/// Time
void Origin::setTime(const double time) noexcept
{
    auto iTimeMuS = static_cast<int64_t> (std::round(time*1.e6));
    setTime(std::chrono::microseconds {iTimeMuS});
}

void Origin::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Origin::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not set");}
    return pImpl->mTime;
}

bool Origin::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Latitude
void Origin::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90) 
    {   
        throw std::invalid_argument("Latitude must be in [-90,90]");
    }   
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double Origin::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool Origin::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void Origin::setLongitude(const double lonIn) noexcept
{
    pImpl->mLongitude = ::lonTo180(lonIn);
    pImpl->mHaveLongitude = true;
}
    
double Origin::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool Origin::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}

/// Depth
void Origin::setDepth(const double depth)
{
    if (depth < -8600 || depth > 800000)
    {   
        throw std::invalid_argument("Depth must be in range [-8600,800000]");
    }   
    pImpl->mDepth = depth;
    pImpl->mHaveDepth = true;
}

double Origin::getDepth() const
{
    if (!haveDepth()){throw std::runtime_error("Depth not set");}
    return pImpl->mDepth;
}

bool Origin::haveDepth() const noexcept
{
    return pImpl->mHaveDepth;
}

/// Preferred magnitude
void Origin::setPreferredMagnitude(const IMagnitude &magnitude)
{
    if (!magnitude.haveType())
    {
        throw std::invalid_argument("Magnitude type not set");
    }
    if (!magnitude.haveSize())
    {
        throw std::invalid_argument("Magnitude size not set");
    }
//    pImpl->mPreferredMagnitude = magnitude.clone();
    pImpl->mHavePreferredMagnitude = true;
}

bool Origin::havePreferredMagnitude() const noexcept
{
    return pImpl->mHavePreferredMagnitude;
}

/// Arrivals
/// Set arrivals
void Origin::setArrivals(const std::vector<Arrival> &arrivals)
{
    // Only add valid stations
    pImpl->mArrivals.clear();
    pImpl->mArrivals.reserve(arrivals.size());
    auto nArrivals = static_cast<int> (arrivals.size());
    for (int i = 0; i < nArrivals; ++i)
    {   
        if (!arrivals[i].haveNetwork())
        {
            spdlog::warn("Network not set; skipping");
            continue;
        }
        if (!arrivals[i].haveStation())
        {
            spdlog::warn("Station not set; skipping");
            continue;
        }
/*
        if (!arrivals[i].haveChannels())
        {
            spdlog::warn("Channels not set; skipping");
            continue;
        }
*/
        if (!arrivals[i].haveLocationCode())
        {
            spdlog::warn("Location code not set; skipping");
            continue;
        }
        if (!arrivals[i].haveTime())
        {
            spdlog::warn("Time not set; skipping");
            continue;
        }
        if (!arrivals[i].havePhase())
        {
            spdlog::warn("Phase not set; skipping");
            continue;
        }
        pImpl->mArrivals.push_back(arrivals[i]);
    }
}

const std::vector<Arrival> &Origin::getArrivalsReference() const noexcept
{
    return *&pImpl->mArrivals;
}

/// Event type
void Origin::setEventType(const EventType type) noexcept
{
    pImpl->mEventType = type;
}

Origin::EventType Origin::getEventType() const noexcept
{
    return pImpl->mEventType;
}

/// To object
nlohmann::json MLReview::Service::Catalog::toObject(const Origin &origin)
{
    nlohmann::json result;
    result["time"] = origin.getTime().count()*1.e-6;
    result["latitude"] = origin.getLatitude();
    result["longitude"] = origin.getLongitude();
    result["depth"] = origin.getDepth();
    result["eventType"] = ::eventTypeToString(origin.getEventType()); 
    if (origin.havePreferredMagnitude())
    {
 
    }
    const auto &arrivals = origin.getArrivalsReference();
    if (!arrivals.empty())
    {
        nlohmann::json arrivalObjects;
        for (const auto &arrival : arrivals)
        {
            try
            {
                arrivalObjects.push_back(toObject(arrival));
            }
            catch (std::exception &e)
            {
                spdlog::warn(e.what());
            }
        }
        if (!arrivalObjects.empty())
        {
            result["arrivals"] = arrivalObjects;
        }
    }
    return result;
}
