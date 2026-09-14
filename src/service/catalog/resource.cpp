#include <string>
#include <atomic>
#include <mutex>
#include <cmath>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include "mlReview/service/catalog/resource.hpp"
#include "mlReview/service/catalog/response.hpp"
#include "mlReview/service/catalog/event.hpp"
#include "mlReview/service/catalog/origin.hpp"
#include "mlReview/service/catalog/arrival.hpp"
//#include "mlReview/database/connection/postgresql.hpp"
#include "mlReview/database/connection/mongodb.hpp"
#include "mlReview/messages/error.hpp"
#ifdef WITH_SFF
#include "sff/utilities/time.hpp"
#include "sff/hypoinverse2000/eventSummary.hpp"
#include "sff/hypoinverse2000/eventSummaryLine.hpp"
#include "sff/hypoinverse2000/stationArchiveLine.hpp"
#endif

#define RESOURCE_NAME "catalog"
#define COLLECTION_NAME "events"

using namespace MLReview::Service::Catalog;

namespace
{


std::chrono::seconds now()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>
           (now.time_since_epoch());
}

#ifdef WITH_SFF
std::string eventToHypoInverse2000(const Event &event)
{
    SFF::HypoInverse2000::EventSummary eventSummary; 
    SFF::HypoInverse2000::EventSummaryLine eventSummaryLine;
    auto origin = event.getPreferredOrigin();
    auto originTime = origin.getTime().count()*1.e-6;
    eventSummaryLine.setOriginTime(SFF::Utilities::Time {originTime});
    eventSummaryLine.setLatitude(origin.getLatitude());
    eventSummaryLine.setLongitude(origin.getLongitude());
    eventSummaryLine.setDepth(origin.getDepth()*1.e-3);
    eventSummaryLine.setPreferredMagnitude(1);
    eventSummaryLine.setPreferredMagnitudeLabel('l');
    int nP{0};
    int nS{0};
    int nFM{0};
    auto arrivals = origin.getArrivalsReference();
    for (const auto &arrival : arrivals)
    {
        try
        {
            SFF::HypoInverse2000::StationArchiveLine hypoArrival;
            hypoArrival.setNetworkName(arrival.getNetwork());
            hypoArrival.setStationName(arrival.getStation());
            if (arrival.haveLocationCode())
            {
                hypoArrival.setLocationCode(arrival.getLocationCode());
            }
            auto arrivalTime = arrival.getTime().count()*1.e-6;
            auto residual = arrival.getResidual();
            if (arrivalTime < originTime)
            {
                throw std::runtime_error("arrival time less than origin time");
            }
            if (arrival.getPhase() == "P")
            {
                hypoArrival.setChannelName(arrival.getVerticalChannel());
                hypoArrival.setPPickTime(SFF::Utilities::Time {arrivalTime});
                hypoArrival.setPRemark("P");
                hypoArrival.setPWeightCode(1);
                if (residual){hypoArrival.setPResidual(*residual);}
                eventSummary.addPPick(hypoArrival);
                nP = nP + 1;
            }
            else if (arrival.getPhase() == "S")
            {
                auto nonVerticalChannels = arrival.getNonVerticalChannels();
                if (nonVerticalChannels)
                {
                    hypoArrival.setChannelName(nonVerticalChannels->first);
                }
                else
                {
                    hypoArrival.setChannelName(arrival.getVerticalChannel());
                }
                hypoArrival.setSPickTime(SFF::Utilities::Time {arrivalTime});
                hypoArrival.setSRemark("S");
                hypoArrival.setSWeightCode(2);
                if (residual){hypoArrival.setSResidual(*residual);}
                eventSummary.addSPick(hypoArrival);
                nS = nS + 1;
            }
        }
        catch (const std::exception &e)
        {
            spdlog::warn("Failed to add hypoinverse arrival because: "
                       + std::string{e.what()});
        }
    }
    eventSummaryLine.setNumberOfWeightedResiduals(nP + nS);
    eventSummaryLine.setNumberOfFirstMotions(nFM);
    eventSummaryLine.setEventIdentifier(0);//event.getIdentifier());
    eventSummary.setEventInformation(eventSummaryLine);
    return eventSummary.packString();
}
#endif

nlohmann::json toObject(const std::vector<Event> &events)
{
    nlohmann::json result;
    nlohmann::json jsonEvents;
    for (const auto &event : events)
    {
        try
        {
            jsonEvents.push_back(toObject(event));
        }
        catch (const std::exception &e)
        {
            spdlog::warn(e.what());
        }
    }
    result["events"] = jsonEvents;
    size_t hash{0};
    try
    {
        hash = std::hash<nlohmann::json> {}(result);
        result["hash"] = hash;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Failed to create hash: " + std::string {e.what()});
        result["hash"] = hash;
    }
    return result;
}

/// Get the last update
std::chrono::seconds
    getLastUpdate(MLReview::Database::Connection::MongoDB &connection,
                  const std::chrono::seconds &lastUpdate,
                  const std::string &collectionName = COLLECTION_NAME)
{
    std::chrono::seconds result{0};
    try
    {
        auto databaseName = connection.getDatabaseName();
        using namespace bsoncxx::builder::basic;
        auto client
            = reinterpret_cast<mongocxx::client *> (connection.getSession());
        auto database = client->database(databaseName);
        if (database)
        {   
            auto collection = database.collection(collectionName);
            if (collection)
            {
                mongocxx::options::find searchOptions{};
                searchOptions.sort(make_document(kvp("lastUpdate", -1))); 
                searchOptions.projection(
                   make_document(kvp("lastUpdate", 1),
                                 kvp("_id", 0))
                );
                auto filterKey
                    = bsoncxx::document::view_or_value(
                         make_document(kvp("lastUpdate",
                                       make_document(kvp("$gt", lastUpdate.count())))
                                      ));
                auto foundDocument = collection.find_one(filterKey, searchOptions);
                if (foundDocument)
                {
                    auto json
                       = bsoncxx::to_json(*foundDocument,
                                          bsoncxx::ExtendedJsonMode::k_relaxed);
                    auto jsonObject = nlohmann::json::parse(json);
                    result
                       = std::chrono::seconds {
                            jsonObject["lastUpdate"].template get<int64_t> ()
                         }; 
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Could not get update time because: "
                   + std::string {e.what()});
    }
    return result;
}

/// Generates a catalog from the application database
std::pair<std::chrono::seconds, std::vector<Event>>
getEventsFromMongoDB(MLReview::Database::Connection::MongoDB &connection,
                     const std::chrono::seconds &startTime
                        = now() - std::chrono::seconds {86400*14},
                     const int maxEvents = 8192,
                     const std::string collectionName = COLLECTION_NAME)
{
    std::chrono::seconds lastUpdate{0};
    std::vector<Event> events;
    auto databaseName = connection.getDatabaseName();
    using namespace bsoncxx::builder::basic;
    auto client
        = reinterpret_cast<mongocxx::client *> (connection.getSession());
    auto database = client->database(databaseName);
    if (database)
    {   
        auto collection = database.collection(collectionName);
        if (collection)
        {
            mongocxx::options::find searchOptions{};
            searchOptions.sort(make_document(kvp("eventIdentifier", 1))); 
            searchOptions.projection(
                make_document(kvp("waveformData", 0),
                              kvp("_id", 0))
            );
            auto filterKey
                = bsoncxx::document::view_or_value(
                     make_document(kvp("loadDate",
                                   make_document(kvp("$gt", startTime.count())))
                                  ));
            auto cursorFiltered = collection.find(filterKey, searchOptions);
            for (const auto &document : cursorFiltered)
            {
                try
                {
                    auto json
                       = bsoncxx::to_json(document,
                                          bsoncxx::ExtendedJsonMode::k_relaxed);
                    auto jsonObject = nlohmann::json::parse(json);

                    lastUpdate 
                        = std::max(lastUpdate,
                                   std::chrono::seconds {jsonObject["lastUpdate"].template get<int64_t> ()});

                    Event event{jsonObject};
                    events.push_back(std::move(event));
/*

                    Event event;
                    event.setIdentifier(jsonObject["eventIdentifier"].template get<int64_t> ());
                    if (jsonObject.contains("aqmsEventIdentifiers"))
                    {
                        auto aqmsEventIdentifiers = jsonObject["aqmsEventIdentifiers"].template get<std::vector<int64_t>> ();
                        event.setAQMSEventIdentifiers(aqmsEventIdentifiers);
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
                        origin.setArrivals(arrivals);
                    }

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
 
                    event.setPreferredOrigin(origin);
                    //std::cout << eventToHypoInverse2000(event) << std::endl;
                    events.push_back(std::move(event));
*/
                }
                catch (const std::exception &e)
                {
                    spdlog::warn(e.what());
                    continue;
                }
                if (static_cast<int> (events.size()) > maxEvents)
                {
                    spdlog::warn("Exceeded maximum number of events");
                    break;
                }
            }
        }
        else
        {
            spdlog::warn("No collection named " + collectionName);
        }
    }
    else
    {
        spdlog::warn("No database named " + databaseName);
    }
    return std::pair {lastUpdate, events};
}

}

class Resource::ResourceImpl
{
public:
    ResourceImpl(
        std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
        mMongoDBConnection(mongoConnection)
    {
spdlog::warn("Need to make polling thread pop events over 2 weeks old");
        /*
        mEvents = getEvents(*mMongoDBConnection);
        mEventsJSON = ::toObject(mEvents);
        mHash = std::hash<nlohmann::json> {}(mEventsJSON);
        */
        updateStandardCatalog();
    }
    [[nodiscard]] bool keepRunning() const noexcept
    {
        return mKeepRunning;
    }
    void updateStandardCatalog()
    {
        auto [lastUpdate, currentEvents] = ::getEventsFromMongoDB(*mMongoDBConnection);
        auto currentEventsJSON = ::toObject(currentEvents);
        auto hash = currentEventsJSON["hash"].template get<size_t> ();
        std::lock_guard<std::mutex> lockGuard(mMutex);
        {
        mLastUpdate = lastUpdate;
        mEvents = std::move(currentEvents); 
        mEventsJSON = std::move(currentEventsJSON);
        mHash = hash;
        }
    }
    [[nodiscard]] nlohmann::json getStandardCatalogJSON() const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mEventsJSON;
    }
    [[nodiscard]] size_t getHash() const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mHash;
    }
    void pollCatalog()
    {
        constexpr std::chrono::seconds queryInterval{30};
        spdlog::info("Beginning catalog polling...");
        auto lastQueryTime = ::now();
        while (true)
        {
            if (!mKeepRunning){break;}
            auto currentTime = ::now();
            if (currentTime > lastQueryTime + queryInterval)
            {
                lastQueryTime = currentTime;
                auto lastUpdate = ::getLastUpdate(*mMongoDBConnection, mLastUpdate);
                if (lastUpdate > mLastUpdate)
                {
                    mLastUpdate = lastUpdate;
                    spdlog::info("Catalog update!");
                    updateStandardCatalog();
                }
                else
                {
                    spdlog::debug("No catalog update; going back to sleep");
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds {1});
        }    
        spdlog::info("Ending catalog polling");
    }
    void start()
    {
        stop();
        mKeepRunning = true;
        mQueryThread = std::thread(&ResourceImpl::pollCatalog, this);
    }

    void stop() 
    {
        mKeepRunning = false;
        if (mQueryThread.joinable()){mQueryThread.join();}
    }
    ~ResourceImpl()
    {
        stop();
    }
    mutable std::mutex mMutex;
    std::thread mQueryThread;
    std::atomic<bool> mKeepRunning{true};
    std::shared_ptr<MLReview::Database::Connection::MongoDB>
        mMongoDBConnection{nullptr};
    //std::shared_ptr<MLReview::Database::Connection::PostgreSQL>
    //    mAQMSConnection{nullptr};
    std::vector<Event> mEvents;
    nlohmann::json mEventsJSON;
    std::chrono::seconds mLastUpdate{0};
    size_t mHash{0};
};

/// Constructor
Resource::Resource(
    std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
    pImpl(std::make_unique<ResourceImpl> (mongoConnection))
{
    pImpl->start();
}

/// Destructor
Resource::~Resource() = default;

/// Resource name
std::string Resource::getName() const noexcept
{
    return RESOURCE_NAME;
}

/// Process request
std::unique_ptr<MLReview::Messages::IMessage> 
Resource::processRequest(const nlohmann::json &request)
{
    // Figure out the query information
    auto now = std::chrono::duration_cast<std::chrono::seconds> (
        std::chrono::system_clock::now().time_since_epoch()); 
    auto endTime = now;
    auto startTime = now - std::chrono::seconds{14*86400};
    bool customQuery{false};
    if (request.contains("startTime"))
    {
        auto startTimeRequest
            = std::chrono::seconds
              {request["startTime"].template get<int64_t> ()};
        if (startTimeRequest != startTime)
        {
            startTime = startTimeRequest;
            customQuery = true;
        }
    }
    if (request.contains("endTime"))
    {   
        auto endTimeRequest
            = std::chrono::seconds
              {request["endTime"].template get<int64_t> ()};
        if (endTimeRequest != endTime)
        {
            endTime = endTimeRequest;
            customQuery = true;
        } 
    }
    // Do the times make sense?
    if (endTime <= startTime)
    {
        throw std::invalid_argument(
            "Catalog end time must be greater than start time");
    }
    std::string format{"json"};
    if (request.contains("format"))
    {
        auto requestFormat
           = request["format"].template get<std::string> ();
        if (requestFormat != format)
        {

        }
        else
        {
            if (requestFormat != "json")
            {
                throw std::invalid_argument("Unhandled catalog format: "
                                          + requestFormat);
            }
        }
    }
    auto response = std::make_unique<Response> ();
    if (!customQuery)
    {
        bool hashOnly{false};
        if (request.contains("hashOnly"))
        {
            hashOnly = request["hashOnly"].template get<bool> ();
        }
        if (hashOnly)
        {
            nlohmann::json result;
            result["hash"] = pImpl->getHash();
            response->setMessage("Successful response to standard catalog hash request");
            response->setData(std::move(result));
        }
        else
        {
            response->setMessage("Successful response to standard catalog request");
            response->setData(std::move(pImpl->getStandardCatalogJSON()));
        }
    }
    else
    {
        response->setMessage("Successful response to custom catalog request");
        response->setData(pImpl->mEventsJSON); // TODO
    } 
    return response;
}
