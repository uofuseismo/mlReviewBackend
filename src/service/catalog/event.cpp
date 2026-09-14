#include <iostream>
#include <cmath>
#include <vector>
#include <spdlog/spdlog.h>
#include "mlReview/service/catalog/event.hpp"
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"

using namespace MLReview::Service::Catalog;

class Event::EventImpl
{
public:
    Origin mPreferredOrigin;
    int64_t mIdentifier{0};
    std::vector<int64_t> mAQMSIdentifiers;
    bool mReviewed{false};
    bool mHaveIdentifier{false};
    bool mHaveAQMSIdentifier{false};
    bool mHavePreferredOrigin{false};
    bool mHaveReviewStatus{false};
    bool mSubmittedToCloudCatalog{false};
    bool mHaveSubmittedToCloudCatalogStatus{false};
};

/// Event
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

/// Construct from object
Event::Event(const nlohmann::json &jsonObject) :
    pImpl(std::make_unique<EventImpl> ())
{
    Event event;
    event.setIdentifier(jsonObject["eventIdentifier"].template get<int64_t> ());
    event.toggleReviewed(false);
    if (jsonObject.contains("aqmsEventIdentifiers"))
    {
        auto aqmsEventIdentifiers
            = jsonObject["aqmsEventIdentifiers"].template
              get<std::vector<int64_t>> (); 
        if (!aqmsEventIdentifiers.empty())
        {
            event.setAQMSEventIdentifiers(aqmsEventIdentifiers);
        }
    }
    if (jsonObject.contains("submittedToCloudCatalog"))
    {
        auto submittedToCloudCatalog
            = jsonObject["submittedToCloudCatalog"].template get<bool> ();
        event.toggleSubmittedToCloudCatalog(submittedToCloudCatalog);
    }

    Origin origin;
    const auto &parametricDataObject = jsonObject["parametricData"];
    const auto &preferredOriginObject = parametricDataObject["preferredOrigin"];
    origin.setTime(
        preferredOriginObject["time"].template get<double> ());
    origin.setLatitude(
        preferredOriginObject["latitude"].template get<double> ());
    origin.setLongitude(
        preferredOriginObject["longitude"].template get<double> ());
    origin.setDepth(
        preferredOriginObject["depth"].template get<double> ());

    if (preferredOriginObject.contains("arrivals"))
    {
        std::vector<Arrival> arrivals;
        const auto &arrivalsObject = preferredOriginObject["arrivals"];
        for (const auto &arrivalObject : arrivalsObject)
        {
            try
            {
                Arrival arrival;
                arrival.setNetwork(arrivalObject["network"].template get<std::string> ());
                arrival.setStation(arrivalObject["station"].template get<std::string> ());
                std::string locationCode{"--"};
                if (arrivalObject.contains("channel2") &&
                    arrivalObject.contains("channel3"))
                {
                    arrival.setChannels(arrivalObject["channel1"].template get<std::string> (), 
                                        arrivalObject["channel2"].template get<std::string> (), 
                                        arrivalObject["channel3"].template get<std::string> ());
                }
                else
                {
                    arrival.setChannels(arrivalObject["channel1"].template get<std::string> (), 
                                        "",
                                        "");
                }
                if (arrivalObject.contains("locationCode"))
                {
                    arrival.setLocationCode(arrivalObject["locationCode"].template get<std::string> ());
                }
                arrival.setPhase(arrivalObject["phase"].template get<std::string> ());
                arrival.setTime(arrivalObject["time"].template get<double> ());
                arrival.setResidual(arrivalObject["residual"].template get<double> ());
                arrivals.push_back(std::move(arrival));
            }
            catch (const std::exception &e)
            {
                spdlog::warn("Failed to add arrival to origin; skipping");
            }
        }
        origin.setArrivals(arrivals);
    }
    if (preferredOriginObject.contains("reviewStatus"))
    {   
        auto reviewStatus
            = preferredOriginObject["reviewStatus"].template
              get<std::string> (); 
        if (reviewStatus == "automatic")
        {
            event.toggleReviewed(false);
        }
        else
        {
            event.toggleReviewed(true);
        }
    }   

    // Set the origin
    event.setPreferredOrigin(origin);
    *this = event;
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

/// Event identifier
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

Origin Event::getPreferredOrigin() const
{
    if (!havePreferredOrigin())
    {
        throw std::runtime_error("Preferred origin not set");
    }
    return pImpl->mPreferredOrigin;
}

bool Event::havePreferredOrigin() const noexcept
{
    return pImpl->mHavePreferredOrigin;
}

/// Submitted to cloud catalog?
void Event::toggleSubmittedToCloudCatalog(const bool submitted) noexcept
{
    pImpl->mSubmittedToCloudCatalog = submitted;
    pImpl->mHaveSubmittedToCloudCatalogStatus = true;
}

std::optional<bool> Event::wasSubmittedToCloudCatalog() const noexcept
{
    return pImpl->mHaveSubmittedToCloudCatalogStatus ?
           std::optional<bool> (pImpl->mSubmittedToCloudCatalog) : std::nullopt;
}

/// Reviewed?
void Event::toggleReviewed(const bool reviewed) noexcept
{ 
    pImpl->mReviewed = reviewed;
    pImpl->mHaveReviewStatus = true;
}

std::optional<bool> Event::wasReviewed() const noexcept
{
    return pImpl->mHaveReviewStatus ?
           std::optional<bool> (pImpl->mReviewed) : std::nullopt;
}

/// AQMS identifiers
void Event::setAQMSEventIdentifiers(
    const std::vector<int64_t> &identifiersIn) noexcept
{
    auto identifiers = identifiersIn;
    std::sort(identifiers.begin(), identifiers.end());
    identifiers.erase(
        std::unique(identifiers.begin(), identifiers.end()),
        identifiers.end());
    pImpl->mAQMSIdentifiers = identifiers;
}

std::optional<std::vector<int64_t>>
    Event::getAQMSEventIdentifiers() const noexcept
{
    return !pImpl->mAQMSIdentifiers.empty() ?
           std::optional<std::vector<int64_t>> (pImpl->mAQMSIdentifiers) :
           std::nullopt;
}

/// To object
nlohmann::json MLReview::Service::Catalog::toObject(const Event &event)
{
    nlohmann::json result;
    result["eventIdentifier"] = std::to_string(event.getIdentifier());
    auto reviewed = event.wasReviewed();
    if (reviewed)
    {
        result["reviewed"] = *reviewed;
    }
    result["preferredOrigin"] = toObject(event.getPreferredOrigin());
    auto aqmsEventIdentifiers = event.getAQMSEventIdentifiers();
    if (aqmsEventIdentifiers)
    {
        if (!aqmsEventIdentifiers->empty())
        {
            result["aqmsEventIdentifiers"] = *aqmsEventIdentifiers;
        }
    }
    auto submittedToCloudCatalog = event.wasSubmittedToCloudCatalog();
    if (submittedToCloudCatalog)
    {
        result["submittedToCloudCatalog"] = *submittedToCloudCatalog;
    }
    return result;
}
