#include <string>
#include <mutex>
#include <cmath>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include "mlReview/service/waveforms/resource.hpp"
#include "mlReview/service/waveforms/response.hpp"
#include "mlReview/database/connection/mongodb.hpp"
#include "mlReview/waveServer/waveform.hpp"
#include "mlReview/waveServer/segment.hpp"
#include "mlReview/messages/error.hpp"

#define RESOURCE_NAME "waveforms"
#define COLLECTION_NAME "events"

using namespace MLReview::Service::Waveforms;

namespace
{

std::chrono::seconds now()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>
           (now.time_since_epoch());
}

struct SavedWaveforms
{
    //std::vector<MLReview::WaveServer::Waveform> waveforms;
    nlohmann::json jsonWaveforms;
    std::chrono::seconds lastUpdate{::now()};
};

std::optional<int64_t> getOldestEvent(
    const std::map<int64_t, ::SavedWaveforms> &map)
{
    if (map.empty()){return std::nullopt;}
    int64_t oldestEvent = map.begin()->first;
    auto oldestTime = map.begin()->second.lastUpdate;
    for (const auto &it : map)
    {
        if (it.second.lastUpdate < oldestTime)
        {
            oldestEvent = it.first;
            oldestTime = it.second.lastUpdate;
        }
    }
    return std::optional<int64_t> (oldestEvent);
}

void purgeOldestEventFromMap(std::map<int64_t, ::SavedWaveforms> &map)
{
    if (map.empty()){return;}
    auto oldestEvent = ::getOldestEvent(map);
    if (oldestEvent)
    {
        spdlog::debug("Purging " + std::to_string(*oldestEvent)
                   + " from waveform map");
        map.erase(*oldestEvent);
    }
}

nlohmann::json toObject(const std::vector<MLReview::WaveServer::Waveform> &waveforms)
{
    nlohmann::json result;
    for (const auto &waveform : waveforms)
    {
        try
        {
            result.push_back(toObject(waveform));
        }
        catch (const std::exception &e)
        {
            spdlog::warn(e.what());
        }
    }
    return result;
}

/// Generates a catalog from the application database
std::vector<MLReview::WaveServer::Waveform>
getWaveforms(MLReview::Database::Connection::MongoDB &connection,
             const int64_t identifier,
             const std::string &collectionName = "events")
{
    std::vector<MLReview::WaveServer::Waveform> waveforms;
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
            searchOptions.projection(
                make_document(kvp("parametricData", 0),
                              kvp("_id", 0)) 
            );  
            auto filterKey
                = bsoncxx::document::view_or_value(
                     make_document(kvp("eventIdentifier", identifier)));
            auto foundDocument = collection.find_one(filterKey, searchOptions);
            if (foundDocument)
            {
                try
                {
                    auto json
                       = bsoncxx::to_json(*foundDocument,
                                          bsoncxx::ExtendedJsonMode::k_relaxed);
                    auto jsonObject = nlohmann::json::parse(json);
                    if (jsonObject.contains("waveformData"))
                    {
                        for (const auto &waveformObject : jsonObject["waveformData"])
                        {
                            MLReview::WaveServer::Waveform waveform;
                            auto network = waveformObject["network"].template get<std::string> ();
                            auto station = waveformObject["station"].template get<std::string> ();
                            auto channel = waveformObject["channel"].template get<std::string> ();
                            std::string locationCode{"--"};
                            if (waveformObject.contains("locationCode"))
                            {
                                locationCode = waveformObject["locationCode"].template get<std::string> ();
                            }
                            waveform.setNetwork(network);
                            waveform.setStation(station);
                            waveform.setChannel(channel);
                            waveform.setLocationCode(locationCode); 
                            if (waveformObject.contains("segments"))
                            {
                                for (const auto &segmentObject : waveformObject["segments"])
                                {
                                    MLReview::WaveServer::Segment segment;
                                    auto iStartTime = segmentObject["startTimeMuS"].template get<int64_t> ();
                                    auto samplingRate = segmentObject["samplingRateHZ"].template get<double> ();
                                    segment.setStartTime(std::chrono::microseconds {iStartTime});
                                    segment.setSamplingRate(samplingRate);
                                    auto dataType = segmentObject["dataType"].template get<std::string> ();
                                    if (dataType == "integer32")
                                    {
                                        auto data = segmentObject["data"].template get<std::vector<int>> ();
                                        segment.setData(std::move(data));
                                    }
                                    else if (dataType == "float")
                                    {
                                        auto data = segmentObject["data"].template get<std::vector<float>> (); 
                                        segment.setData(std::move(data));
                                    }
                                    else if (dataType == "integer64")
                                    {
                                        auto data = segmentObject["data"].template get<std::vector<int64_t>> ();
                                        segment.setData(std::move(data));
                                    }
                                    else if (dataType == "double")
                                    {
                                        auto data = segmentObject["data"].template get<std::vector<double>> ();
                                        segment.setData(std::move(data));
                                    }
                                    else
                                    {
                                        spdlog::warn("Unhandled data type: " + dataType);
                                    }
                                    waveform.addSegment(std::move(segment)); 
                                }
                            }
                            // Already exists?
                            bool exists{false};
                            for (const auto &existingWaveform : waveforms)
                            {
                                if (existingWaveform.getNetwork() == waveform.getNetwork() &&
                                    existingWaveform.getStation() == waveform.getStation() &&
                                    existingWaveform.getChannel() == waveform.getChannel() &&
                                    existingWaveform.getLocationCode() == waveform.getLocationCode())
                                {
                                    exists = true;
                                    break;
                                }
                            }
                            if (!exists){waveforms.push_back(std::move(waveform));}
                        }
                    }
                    else
                    {
                        spdlog::warn("No waveforms for event "
                                   + std::to_string(identifier));
                        return waveforms;
                    }
                    return waveforms;
                }
                catch (const std::exception &e)
                {
                    spdlog::warn(e.what());
                }
            } // Loop on documents
            else
            {
                throw std::invalid_argument("No events found with eventIdentifier = " 
                                          + std::to_string(identifier));
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
    return waveforms;
}

}

class Resource::ResourceImpl
{
public:
    ResourceImpl(
        std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
        mMongoDBConnection(mongoConnection)
    {
        if (mMongoDBConnection == nullptr)
        {
            throw std::invalid_argument("MongoDB connection is NULL");
        }
        //getWaveforms(*mMongoDBConnection, 11861);
        //mEvents = getEvents(*mMongoDBConnection);
        //mEventsJSON = ::toObject(mEvents);
    }
    ~ResourceImpl()
    {
        stop();
    }
    void stop()
    {
        if (mQueryThread.joinable()){mQueryThread.join();}
    }
    void cleanMap( )
    {

    }
    [[nodiscard]] bool contains(const int64_t identifier) const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mSavedWaveformsMap.contains(identifier);
    }
    [[nodiscard]] nlohmann::json //std::vector<MLReview::WaveServer::Waveform>
        queryAndUpdateWaveforms(const int64_t identifier)
    {
        std::vector<MLReview::WaveServer::Waveform> waveforms;
        nlohmann::json jsonWaveforms;
        if (contains(identifier))
        {
            {
            std::lock_guard<std::mutex> lockGuard(mMutex);
            jsonWaveforms = mSavedWaveformsMap[identifier].jsonWaveforms;
            }
            return jsonWaveforms;
        } 
        // Update
        try
        {
            waveforms 
                = ::getWaveforms(*mMongoDBConnection,
                                 identifier,
                                 mCollectionName);
            jsonWaveforms = ::toObject(waveforms);
        }
        catch (const std::invalid_argument &e)
        {
            throw std::invalid_argument(e.what());
        }
        catch (const std::exception &e)
        {
            spdlog::error("Failed to perform waveform mongodb query for "
                        + std::to_string(identifier) + ".  Failed with: "
                        + std::string {e.what()});
            throw std::runtime_error("Failed to find waveforms for "
                                   + std::to_string (identifier));
        }

        ::SavedWaveforms savedWaveforms{jsonWaveforms, now()};
        {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        auto insertLocation = mSavedWaveformsMap.find(identifier);
        if (insertLocation == mSavedWaveformsMap.end())
        {
            if (mSavedWaveformsMap.size() > mMaxNumberOfEvents)
            {
                try
                {
                    ::purgeOldestEventFromMap(mSavedWaveformsMap);
                }
                catch (const std::exception &e)
                {
                    spdlog::critical(
                        "Failed to purge oldest event; failed with "
                       + std::string {e.what()});
                }
            }
            mSavedWaveformsMap.insert(
                std::pair {identifier, std::move(savedWaveforms)});
        }
        else
        {
            insertLocation->second = std::move(savedWaveforms);
        }
        }
        return jsonWaveforms;
    }
//private:
    mutable std::mutex mMutex;
    std::thread mQueryThread;
    std::map<int64_t, ::SavedWaveforms> mSavedWaveformsMap;
    std::shared_ptr<MLReview::Database::Connection::MongoDB>
        mMongoDBConnection{nullptr};
    std::string mCollectionName{COLLECTION_NAME};
    size_t mMaxNumberOfEvents{32};
};

/// Constructor
Resource::Resource(
    std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
    pImpl(std::make_unique<ResourceImpl> (mongoConnection))
{
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
    int64_t identifier{-1};
    if (!request.contains("identifier"))
    {
        throw std::invalid_argument("Event identifier not set");
    }
    identifier = request["identifier"].template get<int64_t> ();
    //auto waveforms = pImpl->queryAndUpdateWaveforms(identifier); // Throws //::getWaveforms(*pImpl->mMongoDBConnection, identifier);
    auto jsonWaveforms = pImpl->queryAndUpdateWaveforms(identifier); 
    auto response = std::make_unique<Response> ();
    response->setMessage(
        "Successful response to waveforms request for event "
       + std::to_string(identifier));
    response->setData(std::move(jsonWaveforms));
    //response->setData(::toObject(waveforms));
    return response;
}
