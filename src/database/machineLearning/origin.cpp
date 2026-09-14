#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "mlReview/database/machineLearning/origin.hpp"
#include "mlReview/database/machineLearning/arrival.hpp"
#include "private/lonTo180.hpp"

using namespace MLReview::Database::MachineLearning::RealTime;

class Origin::OriginImpl
{
public:
    std::vector<Arrival> mArrivals;
    std::string mAlgorithm;
    std::chrono::microseconds mTime;
    double mLatitude;
    double mLongitude;
    double mDepth;
    int64_t mIdentifier;
    ReviewStatus mReviewStatus{ReviewStatus::Automatic};
    bool mHaveTime{false};
    bool mHaveLongitude{false};
    bool mHaveLatitude{false};
    bool mHaveDepth{false};   
    bool mHaveIdentifier{false};
    bool mHaveReviewStatus{false};
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

/// Identifier
void Origin::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

int64_t Origin::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not set");}
    return pImpl->mIdentifier;
}

bool Origin::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

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

/// Review status
void Origin::setReviewStatus(ReviewStatus status) noexcept
{
    pImpl->mReviewStatus = status;
    pImpl->mHaveReviewStatus = true;
}

std::optional<Origin::ReviewStatus> Origin::getReviewStatus() const noexcept
{
    return pImpl->mHaveReviewStatus ?
           std::optional<Origin::ReviewStatus> (pImpl->mReviewStatus) :
           std::nullopt;
}

/// Algorithm
void Origin::setAlgorithm(const std::string &algorithm)
{
    if (algorithm.empty()){throw std::invalid_argument("Algorithm is empty");}
    pImpl->mAlgorithm = algorithm;
}

std::optional<std::string> Origin::getAlgorithm() const noexcept
{
    return !pImpl->mAlgorithm.empty() ?
           std::optional<std::string> (pImpl->mAlgorithm) :
           std::nullopt;
}

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
        if (!arrivals[i].haveChannels())
        {
            spdlog::warn("Channels not set; skipping");
            continue;
        }
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
 
        bool duplicate{false};
        auto network = arrivals[i].getNetwork();
        auto station = arrivals[i].getStation();
        auto phase = arrivals[i].getPhase();
        for (int j = i + 1; j < nArrivals; ++j)
        {
            if (arrivals[j].getNetwork() == network &&
                arrivals[j].getStation() == station &&
                arrivals[j].getPhase() == phase)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
        {
            auto stringPhase = "P";
            if (phase == Arrival::Phase::S){stringPhase = "S";}
            spdlog::warn("Will not add " 
                       + network + "." + station + "." + stringPhase);
            continue;
        }
        pImpl->mArrivals.push_back(arrivals[i]);
    }
}

const std::vector<Arrival> &Origin::getArrivalsReference() const noexcept
{
    return *&pImpl->mArrivals;
}

std::vector<Arrival> Origin::getArrivals() const noexcept
{
    return pImpl->mArrivals;
}

/// JSON
nlohmann::json MLReview::Database::MachineLearning::RealTime::toObject(
    const Origin &origin)
{
    nlohmann::json result;
    result["time"] = origin.getTime().count()*1.e-6;
    result["latitude"] = origin.getLatitude();
    result["longitude"] = origin.getLongitude();
    result["depth"] = origin.getDepth();
    result["identifier"] = origin.getIdentifier();
    auto reviewStatus = origin.getReviewStatus();
    if (reviewStatus)
    {
        if (*reviewStatus == Origin::ReviewStatus::Automatic)
        {
            result["reviewStatus"] = "automatic";
        }
        else if (*reviewStatus == Origin::ReviewStatus::Human)
        {
            result["reviewStatus"] = "human";
        }
        else if (*reviewStatus == Origin::ReviewStatus::Finalized)
        {
            result["reviewStatus"] = "finalized";
        }
    }
    auto algorithm = origin.getAlgorithm();
    if (algorithm)
    {
        result["algorithm"] = *algorithm;
    }
    const auto &arrivals = origin.getArrivalsReference();
    if (!arrivals.empty())
    {
        nlohmann::json arrivalObjects;
        for (const auto &arrival : arrivals)
        {
            try
            {
                auto arrivalObject = toObject(arrival);
                arrivalObjects.push_back(std::move(arrivalObject));
            }
            catch (...)
            {
            }
        }
        if (!arrivalObjects.empty())
        {
            result["arrivals"] = arrivalObjects;
        }
    }
    return result;
}
