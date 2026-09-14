#ifndef MONGO_UTILITIES_HPP
#define MONGO_UTILITIES_HPP
#include <string>
#include <chrono>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include "mlReview/database/connection/mongodb.hpp"

#define COLLECTION_NAME "events"

namespace
{

std::chrono::seconds now()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds> (now.time_since_epoch());
}


/// @result True indicates the event with the given identifier exists in
///         the MongoDB (webapp) database.
bool checkIfEventExists(
    MLReview::Database::Connection::MongoDB &connection,
    const int64_t identifier,
    const std::string &collectionName)
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
            searchOptions.projection(
                make_document(kvp("parametricData", 0), 
                              kvp("_id", 0)) 
            );
            auto filterKey
                = bsoncxx::document::view_or_value(
                     make_document(kvp("eventIdentifier", identifier)));
            auto foundDocument = collection.find_one(filterKey, searchOptions);
            return foundDocument ? true : false;
        }
    }   
    else
    {   
        throw std::runtime_error("Database connection is null");
    }   
    return false;
}

/// @brief Let MongoDB know the event was submitted to AWS.
void updateEventSubmittedInMongoDB(
    MLReview::Database::Connection::MongoDB &connection,
    const int64_t mongoIdentifier,
    const std::string &collectionName,
    const bool submitted)
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
            searchOptions.projection(
                make_document(kvp("waveformDataData", 0), 
                              kvp("_id", 0)) 
            );
            auto filterKey
                = bsoncxx::document::view_or_value(
                     make_document(kvp("eventIdentifier", mongoIdentifier)));
            auto foundDocument = collection.find_one(filterKey, searchOptions);
            if (!foundDocument)
            {
                throw std::runtime_error("Could not find event "
                                       + std::to_string(mongoIdentifier));
            }
            // Update
            mongocxx::options::update updateOptions;
            updateOptions.upsert(false);
            nlohmann::json object;
            object["submittedToCloudCatalog"] = submitted;
            object["reviewStatus"] = "human";
            object["lastUpdate"] = static_cast<int64_t> (::now().count());
            bsoncxx::document::value bsonObject
                = bsoncxx::from_json(object.dump());
            auto updateDocument
                = make_document(kvp("$set", bsonObject));
            auto updateResult
                = collection.update_one(filterKey.view(),
                                        updateDocument.view(),
                                        updateOptions);
            spdlog::info("Updated "
                       + std::to_string(mongoIdentifier));
        }
        else
        {
            throw std::runtime_error("Collection "
                                   + collectionName + " does not exist");
        }
    }
    else
    {
        throw std::runtime_error("Database connection is null");
    }   
}

[[nodiscard]]
nlohmann::json getParametricData(
    MLReview::Database::Connection::MongoDB &connection,
    const int64_t mongoIdentifier,
    const std::string &collectionName)
{
    nlohmann::json jsonObject;
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
                make_document(//kvp("parametricData", 1),
                              kvp("waveformData", 0), // Don't get waveforms
                              kvp("_id", 0)) 
            );
            auto filterKey
                = bsoncxx::document::view_or_value(
                     make_document(kvp("eventIdentifier", mongoIdentifier)));
            auto foundDocument = collection.find_one(filterKey, searchOptions);
            if (foundDocument)
            {   
                auto json
                   = bsoncxx::to_json(*foundDocument,
                                      bsoncxx::ExtendedJsonMode::k_relaxed);
                jsonObject = nlohmann::json::parse(json);
            }   
            else
            {   
                throw std::runtime_error("Could not find event "
                                       + std::to_string(mongoIdentifier));
            }   
        }
        else
        {
            throw std::runtime_error("Collection " 
                                   + collectionName + " does not exist");
        }
    }   
    else
    {   
        throw std::runtime_error("Database connection is null");
    }   
    return jsonObject;
}

}
#endif
