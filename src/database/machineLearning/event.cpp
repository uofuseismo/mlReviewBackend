#include <string>
#include <vector>
#include <chrono>
#include "mlReview/database/machineLearning/event.hpp"
#include "mlReview/database/machineLearning/origin.hpp"

using namespace MLReview::Database::MachineLearning::RealTime;

class Event::EventImpl
{
public:
    Origin mPreferredOrigin;
    std::string mAuthority;
    int64_t mIdentifier;
    Type mType{Type::Unknown};
    MonitoringRegion mMonitoringRegion{MonitoringRegion::Unknown};
    bool mHavePreferredOrigin{false};
    bool mHaveIdentifier{false};
};

/// Constructor
Event::Event() :
    pImpl(std::make_unique<EventImpl> ())
{
}

/// Copy constructor
Event::Event(const Event &event)
{
    *this = event;
}

/// Move constructor
Event::Event(Event &&event) noexcept
{
    *this = std::move(event);
}

/// Copy assignment
Event& Event::operator=(const Event &event)
{
    if (&event == this){return *this;}
    pImpl = std::make_unique<EventImpl> (*event.pImpl);
    return *this;
}

/// Move assignment
Event& Event::operator=(Event &&event) noexcept
{
    if (&event == this){return *this;}
    pImpl = std::move(event.pImpl);
    return *this;
}

/// Reset class 
void Event::clear() noexcept
{
    pImpl = std::make_unique<EventImpl> ();
}   

/// Destructor
Event::~Event() = default;

/// Identifier
void Event::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

int64_t Event::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not set");}
    return pImpl->mIdentifier;
}

bool Event::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Type
void Event::setType(const Type type) noexcept
{
    pImpl->mType = type;
}

Event::Type Event::getType() const noexcept
{
    return pImpl->mType;
}

/// Monitoring region
void Event::setMonitoringRegion(const MonitoringRegion region) noexcept
{
    pImpl->mMonitoringRegion = region;
}

Event::MonitoringRegion Event::getMonitoringRegion() const noexcept
{
    return pImpl->mMonitoringRegion;
}

/// Authority
void Event::setAuthority(const std::string &authority)
{
    if (authority.empty()){throw std::runtime_error("Authority is empty");}
    pImpl->mAuthority = authority;
}

std::optional<std::string> Event::getAuthority() const noexcept
{
    return !pImpl->mAuthority.empty() ? 
           std::optional<std::string> (pImpl->mAuthority) :
           std::nullopt;
}

/// Set the preferred origin
void Event::setPreferredOrigin(const Origin &origin)
{
    if (!origin.haveTime())
    {
        throw std::invalid_argument("Time not set");
    }
    if (!origin.haveLatitude())
    {
        throw std::invalid_argument("Latitude not set");
    }
    if (!origin.haveLongitude())
    {
        throw std::invalid_argument("Longitude not set");
    }
    if (!origin.haveDepth())
    {
        throw std::invalid_argument("Depth not set");
    }
    pImpl->mPreferredOrigin = origin;
    pImpl->mHavePreferredOrigin = true;
}

std::optional<Origin> Event::getPreferredOrigin() const noexcept
{
    return pImpl->mHavePreferredOrigin ?
           std::optional<Origin> (pImpl->mPreferredOrigin) :
           std::nullopt;
}

/// JSON
nlohmann::json MLReview::Database::MachineLearning::RealTime::toObject(
    const Event &event)
{
    nlohmann::json result;
    result["identifier"] = event.getIdentifier();
    std::string type{"unknown"};
    if (event.getType() == Event::Type::Earthquake)
    {
        type = "earthquake";
    }
    else if (event.getType() == Event::Type::QuarryBlast)
    {
        type = "quarryBlast";
    }
    else
    {
        if (event.getType() != Event::Type::Unknown)
        {
            throw std::runtime_error("Unhandled event type");
        }
    } 
    result["eventType"] = type;
    std::string monitoringRegion{"unknown"}; 
    if (event.getMonitoringRegion() == Event::MonitoringRegion::Yellowstone)
    {
        monitoringRegion = "yellowstone";
    }
    else if (event.getMonitoringRegion() == Event::MonitoringRegion::Utah)
    {
        monitoringRegion = "utah";
    }
    else
    {
        if (event.getMonitoringRegion() != Event::MonitoringRegion::Unknown)
        {
            throw std::runtime_error("Unhandled monitoring region");
        }
    }
    result["monitoringRegion"] = monitoringRegion;
    auto preferredOrigin = event.getPreferredOrigin();
    if (preferredOrigin)
    {
        result["preferredOrigin"] = toObject(*preferredOrigin);
    }
    auto authority = event.getAuthority();
    if (authority)
    {
        result["authority"] = *authority;
    }
    return result;
}
