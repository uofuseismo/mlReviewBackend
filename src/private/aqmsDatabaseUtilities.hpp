#ifndef PRIVATE_AQMS_DATABASE_UTILITIES_HPP
#define PRIVATE_AQMS_DATABASE_UTILITIES_HPP
#include <cmath>
#include <vector>
#include <string>
#include <optional>
#include <soci/soci.h>
#include <spdlog/spdlog.h>
#include "private/regions.hpp"
#include "mlReview/service/catalog/event.hpp"
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"
#include "mlReview/database/connection/postgresql.hpp"
#include "mlReview/database/aqms/event.hpp"
#include "mlReview/database/aqms/credit.hpp"
#include "mlReview/database/aqms/origin.hpp"
#include "mlReview/database/aqms/assocaro.hpp"
#include "mlReview/database/aqms/arrival.hpp"


namespace
{

/// Gets the next sequence value
[[nodiscard]]
int64_t getNextSequenceValue(soci::session &session,
                             const std::string &sequenceName)
{
    if (sequenceName.empty())
    {   
        throw std::invalid_argument("sequenceName is empty");
    }   
    int64_t sequenceValue{-1};
    try 
    {   
        {
        soci::transaction tr(session);
        session << "SELECT sequence.getNext(:sequenceName, 1)",
                   soci::use(sequenceName),
                   soci::into(sequenceValue);
        tr.commit();
        }
    }   
    catch (const std::exception &e) 
    {   
        spdlog::warn(e.what());
    }   
    if (sequenceValue < 0)
    {   
        throw std::runtime_error("Failed to get sequence value for "
                               + sequenceName);
    }   
    return sequenceValue;
}

[[nodiscard]]
std::vector<int64_t> getNextSequenceValues(
    soci::session &session,
    const int n,
    const std::string &sequenceName)
{
    if (sequenceName.empty())
    {
        throw std::invalid_argument("sequenceName is empty");
    }   
    if (n < 0)
    {
        throw std::invalid_argument("Number of sequence values must be positive");
    }   
    std::vector<int64_t> sequenceValues;
    if (n == 0){return sequenceValues;}
    // Incrementally build it up
    for (int i = 0; i < n; ++i)
    {
        sequenceValues.push_back(::getNextSequenceValue(session, sequenceName));
    }
    if (static_cast<int> (sequenceValues.size()) != n)
    {   
        throw std::invalid_argument("Failed to get sequence values for "
                                  + sequenceName);
    }   
    return sequenceValues;
}

[[nodiscard]] [[maybe_unused]]
int64_t getNextEventSequenceValue(soci::session &session)
{
    return ::getNextSequenceValue(session, "evseq");
}

[[nodiscard]] [[maybe_unused]]
int64_t getNextOriginSequenceValue(soci::session &session)
{
    return ::getNextSequenceValue(session, "orseq");
}

[[nodiscard]] [[maybe_unused]]
std::vector<int64_t> getNextArrivalSequenceValues(soci::session &session,
                                                  const int n)
{
    return ::getNextSequenceValues(session, n, "arseq");
}

[[nodiscard]]
std::pair<std::vector<MLReview::Database::AQMS::Arrival>,
          std::vector<MLReview::Database::AQMS::AssocArO>>
toArrivalsAndAssociations(const MLReview::Service::Catalog::Event &event,
                          const std::string &authority,
                          const std::string &subSource,
                          const bool isAutomatic)
{
    std::pair<
       std::vector<MLReview::Database::AQMS::Arrival>,
       std::vector<MLReview::Database::AQMS::AssocArO>
    > result;
    std::vector<MLReview::Database::AQMS::Arrival> aqmsArrivals;
    std::vector<MLReview::Database::AQMS::AssocArO> aqmsAssocArOs;
    auto origin = event.getPreferredOrigin();
    const auto &arrivals = origin.getArrivalsReference();
    for (const auto &arrival : arrivals)
    {
        MLReview::Database::AQMS::Arrival aqmsArrival;
        MLReview::Database::AQMS::AssocArO assocaro;
        try
        {
            aqmsArrival.setNetwork(arrival.getNetwork());
            aqmsArrival.setStation(arrival.getStation());
            auto phase = arrival.getPhase();
            auto verticalChannel = arrival.getVerticalChannel();
            double weight{1};
            if (phase == "P")
            {
                aqmsArrival.setSEEDChannel(verticalChannel);
                aqmsArrival.setPhase(phase);
                aqmsArrival.setQuality(0.75);
                weight = 1;
            }
            else if (phase == "S")
            {
                auto nonVerticalChannels = arrival.getNonVerticalChannels();
                if (nonVerticalChannels)
                {
                    aqmsArrival.setSEEDChannel(nonVerticalChannels->first);
                }
                else
                {
                    aqmsArrival.setSEEDChannel(verticalChannel);
                }
                aqmsArrival.setPhase(phase);
                aqmsArrival.setQuality(0.5);
                weight = 0.5;
            }
            else
            {
                throw std::runtime_error("Unhandled phase");
            }
            if (arrival.haveLocationCode())
            {
                auto locationCode = arrival.getLocationCode();
                if (locationCode != "--")
                {
                    aqmsArrival.setLocationCode(locationCode);
                }
            }
            aqmsArrival.setTime(arrival.getTime());
            aqmsArrival.setSubSource(subSource);
            aqmsArrival.setAuthority(authority);
            aqmsArrival.setQuality(weight);
            aqmsArrival.setReviewFlag(MLReview::Database::AQMS::Arrival::ReviewFlag::Automatic);
            if (!isAutomatic)
            {
                aqmsArrival.setReviewFlag(MLReview::Database::AQMS::Arrival::ReviewFlag::Human);
            }
            assocaro.setAuthority(authority);
            assocaro.setSubSource(subSource);
            assocaro.setPhase(phase); 
            auto residual = arrival.getResidual();
            if (residual){assocaro.setTravelTimeResidual(*residual);}
            aqmsArrivals.push_back(std::move(aqmsArrival));
            aqmsAssocArOs.push_back(std::move(assocaro));
        }
        catch (const std::exception &e) 
        {
            spdlog::warn("Failed to add arrival because "
                       + std::string {e.what()});
        }
    }   
    return std::pair{aqmsArrivals, aqmsAssocArOs};
}

[[nodiscard]]
MLReview::Database::AQMS::Origin
    toOrigin(const MLReview::Service::Catalog::Event &event,
             const std::string &authority,
             const std::string &subSource,
             const std::string &algorithm,
             const bool isAutomatic)
{
    MLReview::Database::AQMS::Origin result;
    auto origin = event.getPreferredOrigin();
    auto latitude = origin.getLatitude();
    auto longitude = origin.getLongitude();
    result.setTime(origin.getTime());
    result.setLatitude(latitude);
    result.setLongitude(longitude);
    result.setDepth(origin.getDepth()*1.e-3);
    result.setAuthority(authority);
    result.setSubSource(subSource);
    auto isLocal = isInAuthoritativeRegion(latitude, longitude);
    if (isLocal)
    {   
        result.setGeographicType(
            MLReview::Database::AQMS::Origin::GeographicType::Local);
    }   
    else
    {   
        result.setGeographicType(
            MLReview::Database::AQMS::Origin::GeographicType::Regional); 
    }   
    result.setReviewFlag(MLReview::Database::AQMS::Origin::ReviewFlag::Human);
    if (isAutomatic)
    {   
        result.setReviewFlag(MLReview::Database::AQMS::Origin::ReviewFlag::Automatic);
    }   
    return result;
}

[[nodiscard]]
MLReview::Database::AQMS::Event 
    toEvent(const MLReview::Service::Catalog::Event &event,
            const bool isEarthquake,
            const std::string &authority,
            const std::string &subSource)
{
    MLReview::Database::AQMS::Event result;
    result.setAuthority(authority);
    result.setVersion(0);
    result.setSubSource(subSource);
    result.setSelectFlag();
    result.setType(MLReview::Database::AQMS::Event::Type::Unknown);
    if (isEarthquake)
    {   
        result.setType(MLReview::Database::AQMS::Event::Type::Earthquake);
    }   
    return result;
}


[[nodiscard]] [[maybe_unused]]
std::optional<int64_t>
    matchEventToAQMS(//const MLReview::Service::Catalog::Event &event,
                     const double latitude,
                     const double longitude,
                     const double depth,
                     const double originTime,
                     MLReview::Database::Connection::PostgreSQL &connection,
                     const double originTimeTolerance = 1.5,
                     const double latitudeTolerance = 0.1,
                     const double longitudeTolerance = 0.1)
{
    auto session
        = reinterpret_cast<soci::session *> (connection.getSession());
    if (!connection.isConnected())
    {
        connection.connect();
        if (!connection.isConnected())
        {
            throw std::runtime_error("Not connected to AQMS PG database");
        }
    }
/*
    auto origin = event.getPreferredOrigin();
    auto latitude = origin.getLatitude();
    auto longitude = origin.getLongitude();
    auto depth = origin.getDepth();
    auto originTime = origin.getTime().count()*1.e-6;
*/
    const double startTime{std::floor(originTime - originTimeTolerance)};
    const double endTime{std::ceil(originTime + originTimeTolerance)};
    const std::string query{
R"'''(
SELECT event.evid, origin.lat, origin.lon, origin.depth, TrueTime.getEpoch(origin.datetime, 'NOMINAL') FROM event 
 INNER JOIN origin ON event.prefor = origin.orid 
WHERE event.selectflag = 1 AND TrueTime.getEpoch(origin.datetime, 'NOMINAL') BETWEEN :startTime AND :endTime;
)'''"
    };
    soci::rowset<soci::row> eventRows
       = (session->prepare << query,
                              soci::use(startTime),
                              soci::use(endTime) );
    double maxDistance{std::numeric_limits<double>::max()};
    int64_t closestIdentifier{-1};
    for (soci::rowset<soci::row>::const_iterator it = eventRows.begin();
         it != eventRows.end(); ++it)
    {   
        const auto &row = *it;
        auto otherIdentifier = static_cast<int64_t> (row.get<long long> (0));
        double otherLatitude = row.get<double> (1);
        double otherLongitude = row.get<double> (2);
        double otherDepth = row.get<double> (3);
        double otherOriginTime = row.get<double> (4);
        auto dLatitude = std::abs(otherLatitude - latitude);
        auto dLongitude = std::abs(otherLongitude - longitude);
        auto dDepth = std::abs(otherDepth - depth);
        auto dOriginTime = std::abs(otherOriginTime - originTime);
        // In bounds?
        if (dLatitude < latitudeTolerance &&
            dLongitude < longitudeTolerance &&
            dOriginTime < originTimeTolerance)
        {
            auto candidateDistance2 = dLatitude*dLatitude
                                    + dLongitude*dLongitude
                                    + dDepth*dDepth
                                    + dOriginTime*dOriginTime;
            if (candidateDistance2 < maxDistance)
            {
                maxDistance = candidateDistance2;
                closestIdentifier = otherIdentifier; 
            }
        }
    }   
    return (closestIdentifier > -1) ?
           std::optional<int64_t> (closestIdentifier) : std::nullopt;
}

[[nodiscard]] [[maybe_unused]]
std::optional<int64_t>
    matchEventToAQMS(const MLReview::Service::Catalog::Event &event,
                     MLReview::Database::Connection::PostgreSQL &connection,
                     const double originTimeTolerance = 1.5,
                     const double latitudeTolerance = 0.1,
                     const double longitudeTolerance = 0.1)
{
    auto origin = event.getPreferredOrigin();
    auto latitude = origin.getLatitude(); 
    auto longitude = origin.getLongitude(); 
    auto depth = origin.getDepth();
    auto originTime = origin.getTime().count()*1.e-6;
    return ::matchEventToAQMS(
                              latitude,
                              longitude,
                              depth,
                              originTime,
                              connection,
                              originTimeTolerance,
                              latitudeTolerance,
                              longitudeTolerance);
}

[[nodiscard]] [[maybe_unused]]
int64_t writeToAQMS(const MLReview::Service::Catalog::Event &event,
                    MLReview::Database::Connection::PostgreSQL &connection,
                    const std::string &submitter, // = "mlreview",
                    const std::string &authority,
                    const std::string &subSource,
                    const std::string &originAlgorithm)
{
    if (!connection.isConnected())
    {   
        connection.connect();
        if (!connection.isConnected())
        {
            throw std::runtime_error("Not connected to AQMS PG database");
        }
    }   
    auto session 
        = reinterpret_cast<soci::session *> (connection.getSession());

    // Pack information to AQMS database classes
    constexpr bool isEarthquake{true};
    auto aqmsEvent = ::toEvent(event, isEarthquake, authority, subSource);
    constexpr bool isAutomaticOrigin{false}; // A human accepted it
    auto aqmsOrigin
        = ::toOrigin(event, authority, subSource, originAlgorithm,
                     isAutomaticOrigin);
    constexpr bool isAutomaticPick{true};
    auto [aqmsArrivals, aqmsAssocArOs] 
        = ::toArrivalsAndAssociations(event, authority, subSource,
                                      isAutomaticPick);
    // Get primary key values
    auto eventIdentifier = ::getNextEventSequenceValue(*session);
    auto originIdentifier = ::getNextOriginSequenceValue(*session);
    std::vector<int64_t> arrivalIdentifiers;
    if (!aqmsArrivals.empty())
    {   
        arrivalIdentifiers
            = ::getNextArrivalSequenceValues(*session,
                                             aqmsArrivals.size());
    }   
    if (arrivalIdentifiers.size() != aqmsArrivals.size())
    {   
        throw std::runtime_error("Inconsistent arrival/primary key values");
    }   
    // Update the event information
    aqmsEvent.setIdentifier(eventIdentifier);
    aqmsEvent.setPreferredOriginIdentifier(originIdentifier);
    aqmsOrigin.setIdentifier(originIdentifier);
    aqmsOrigin.setEventIdentifier(eventIdentifier);
    for (int i = 0; i < static_cast<int> (aqmsArrivals.size()); ++i)
    {   
        aqmsAssocArOs[i].setOriginIdentifier(originIdentifier);
        aqmsAssocArOs[i].setArrivalIdentifier(arrivalIdentifiers.at(i));
        aqmsArrivals[i].setIdentifier(arrivalIdentifiers.at(i));
    }   
    auto eventInsertString = MLReview::Database::AQMS::toInsertString(aqmsEvent);
    auto originInsertString = MLReview::Database::AQMS::toInsertString(aqmsOrigin);
    std::vector<std::string> arrivalInsertStrings;
    for (const auto &aqmsArrival : aqmsArrivals)
    {   
        arrivalInsertStrings.push_back(
            MLReview::Database::AQMS::toInsertString(aqmsArrival));
    }   
    std::vector<std::string> assocaroInsertStrings;
    for (const auto &aqmsAssoc : aqmsAssocArOs)
    {   
         assocaroInsertStrings.push_back(
             MLReview::Database::AQMS::toInsertString(aqmsAssoc));
    }   
    spdlog::info("Committing " + std::to_string(eventIdentifier)
               + " to AQMS database");
    {   
    soci::transaction tr(*session);
    *session << eventInsertString;
    *session << originInsertString;
    for (const auto &arrivalInsertString : arrivalInsertStrings)
    {   
        *session << arrivalInsertString;
    }   
    for (const auto &assocaroInsertString : assocaroInsertStrings)
    {   
        *session << assocaroInsertString;
    }   
    tr.commit();
    }   
    spdlog::info("Successfully inserted "
               + std::to_string(eventIdentifier)
               + " into database");
    try 
    {   
        MLReview::Database::AQMS::Credit credit;
        credit.setIdentifier(originIdentifier);
        credit.setTable(MLReview::Database::AQMS::Credit::Table::Origin);
        credit.setReference(submitter);
        auto creditInsertString = MLReview::Database::AQMS::toInsertString(credit);
        {
        soci::transaction tr(*session);
        *session << creditInsertString;
        tr.commit();
        }
        spdlog::info("Successfully inserted origin credit for event "
                   + std::to_string(eventIdentifier));
    }   
    catch (const std::exception &e) 
    {   
        spdlog::warn("Failed to insert origin credit for event "
                   + std::to_string(eventIdentifier));
    }   
    return eventIdentifier;
}

}

#endif
