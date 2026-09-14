#include <string>
#include <sstream>
#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include "mlReview/service/actions/acceptEventToAWS.hpp"
#include "mlReview/messages/message.hpp"
#include "mlReview/messages/error.hpp"
#include "curl.hpp"
#include "mongoUtilities.hpp"

#define RESOURCE_NAME "actions/acceptEventToAWS"

using namespace MLReview::Service::Actions;

namespace
{
class Response final : public MLReview::Messages::IMessage
{
public:
    explicit Response(const std::string &message) :
        mMessage(message)
    {
    }
    int getStatusCode() const noexcept final
    {
        return 200;
    }
    bool getSuccess() const noexcept final
    {
        return true;
    }
    std::optional<std::string> getMessage() const noexcept
    {
        return std::optional<std::string> {mMessage};
    }
    ~Response() final = default;
    std::string mMessage;
};

/*
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
*/

[[nodiscard]]
std::pair<bool, nlohmann::json>
   createCreateUpdateRequest(const nlohmann::json &initialResponse,
                             const std::string &authorityIn = "UU",
                             const bool isHumanReviewed = true)
{
   auto authority = authorityIn;
   std::transform(authority.begin(), authority.end(), authority.begin(),
                  [](auto c){return std::tolower(c);});

   nlohmann::json result;
   auto eventIdentifier = initialResponse["eventIdentifier"].template get<int64_t> ();
   auto parametricData = initialResponse["parametricData"];
   auto preferredOrigin = parametricData["preferredOrigin"];
   result["identifier"] = "urts" + std::to_string(eventIdentifier);
   auto aqmsIdentifiers = initialResponse["aqmsEventIdentifiers"].template get<std::vector<int64_t>> ();
   int64_t aqmsIdentifier{-1};
   if (!aqmsIdentifiers.empty())
   {
       for (auto &identifier : aqmsIdentifiers)
       {
           if (aqmsIdentifier >= 80000000 && aqmsIdentifier < 90000000)
           {
               aqmsIdentifier = identifier;
               break;
           }
       }
   }
   if (aqmsIdentifier >= 80000000)
   {
       result["comcatIdentifier"] = authority + std::to_string(aqmsIdentifier);
   }
   else
   {
       result["comcatIdentifier"] = nullptr;
   }
   result["latitudeDeg"] = preferredOrigin["latitude"].template get<double> ();
   result["longitudeDeg"] = preferredOrigin["longitude"].template get<double> ();
   result["depthKM"] = preferredOrigin["depth"].template get<double> ()*1.e-3;
   result["originTimeUTC"] = preferredOrigin["time"].template get<double> ();
   result["authority"] = authority;
   result["humanReviewed"] = isHumanReviewed;
   result["automaticOrigin"] = true;
   result["locatorAlgorithm"] = preferredOrigin["algorithm"].template get<std::string> ();
   result["catalog"] = "urts";
   result["magnitude"] = nullptr;
   result["magnitudeType"] = nullptr;
   auto eventType = parametricData["eventType"].template get<std::string> ();
   std::transform(eventType.begin(), eventType.end(), eventType.begin(),
                  [](auto c){return std::tolower(c);});
   if (eventType == "earthquake")
   {
       result["eventType"] = "eq"; 
   }
   else if (eventType == "quarryBlast")
   {
       result["eventType"] = "qb";
   }
   else
   {
       spdlog::warn("Unhandled event type: " + eventType);
       result["eventType"] = "uk"; 
   }
   // Arrivals
   nlohmann::json arrivals;
   for (const auto &inputArrival : preferredOrigin["arrivals"])
   {
       nlohmann::json arrival;
       if (inputArrival.contains("network") &&
           inputArrival.contains("station") &&
           inputArrival.contains("phase") &&
           inputArrival.contains("time"))
       {
           auto network = inputArrival["network"].template get<std::string> ();
           auto station = inputArrival["station"].template get<std::string> (); 
           auto phase = inputArrival["phase"].template get<std::string> ();
           auto time = inputArrival["time"].template get<double> ();
           std::string channel;
           if (phase == "P")
           {
               channel = inputArrival["channel1"].template get<std::string> ();
           }
           else if (phase == "S")
           {
               if (inputArrival.contains("channel2"))
               {
                   channel = inputArrival["channel2"].template get<std::string> ();
               }
               else
               {
                   channel = inputArrival["channel1"].template get<std::string> ();
               }
           } 
           else
           {
               spdlog::warn("Unhandled phase " + phase);
               continue;
           }
           auto identifier = network  + station + channel;
           std::string locationCode;
           if (inputArrival.contains("locationCode"))
           {
               locationCode = inputArrival["locationCode"].template get<std::string> ();
               if (!locationCode.empty())
               {
                   identifier = identifier + locationCode;
               }
           }
           identifier = identifier + phase + "-" + std::to_string(static_cast<int64_t> (time*1e6));;
           arrival["identifier"] = identifier;
           arrival["network"] = network;
           arrival["station"] = station;
           arrival["channel"] = channel;
           if (!locationCode.empty())
           {
               arrival["locationCode"] = locationCode;
           }
           else
           {
               arrival["locationCode"] = nullptr;
           }
           arrival["phase"] = phase;
           arrival["timeUTC"] = time;
           arrival["standardErrorS"] = nullptr;
           arrival["algorithm"] = "unet-detection,cnn-refinement";
           arrival["isAutomatic"] = true;
           arrivals.push_back(arrival);
       }
       else
       {
           spdlog::warn("Skipping arrival");
       }
   }
   if (!arrivals.empty()){result["arrivals"] = arrivals;}

   auto monitoringRegion = parametricData["monitoringRegion"].template get<std::string> ();
   std::transform(monitoringRegion.begin(), monitoringRegion.end(), monitoringRegion.begin(),
                  [](auto c){return std::tolower(c);});

   bool isYellowstone{true};
   if (monitoringRegion == "yellowstone")
   {
       isYellowstone = true;
   }
   else if (monitoringRegion == "utah")
   {
       isYellowstone = false;
   }
   else
   {
       throw std::runtime_error("Unhandled monitoring region "
                              + monitoringRegion);
   }
   return std::pair{isYellowstone, result};
}

}


class AcceptEventToAWS::AcceptEventToAWSImpl
{
public:
    AcceptEventToAWSImpl(
        std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
    mMongoDBConnection(mongoConnection)
    {
        if (mMongoDBConnection == nullptr)
        {
            throw std::invalid_argument("MongoDB connection is NULL");
        }   
        auto apiURL = std::getenv("MLREVIEW_AWS_API_URL");
        if (apiURL)
        {
            mAPIURL = std::string {apiURL};
        }
        else
        {
            throw std::runtime_error(
               "Could not read MLREVIEW_AWS_API_URL environment variable");
        }
        if (mAPIURL.empty())
        {
            throw std::runtime_error("MLREVIEW_AWS_API_URL is empty");
        }
        if (mAPIURL.back() != '/'){mAPIURL.push_back('/');}
        auto apiKey = std::getenv("MLREVIEW_AWS_API_ACCESS_KEY");
        if (apiKey)
        {
            mAPIAccessKey = std::string {apiKey};
        }
        else
        {
            throw std::runtime_error(
                "Could not read MLREVIEW_AWS_API_ACCESS_KEY environment variable");
        }
        if (mAPIAccessKey.empty())
        {
            throw std::runtime_error("MLREVIEW_AWS_API_ACCESS_KEY is empty");
        }
    }
    [[nodiscard]]
    nlohmann::json sendRequest(const nlohmann::json &data, bool isYellowstone) const
    {
        std::string uri;
        if (isYellowstone)
        {
            uri = mAPIURL + "Yellowstone";
        }
        else
        {
            uri = mAPIURL + "Utah";
        }
        constexpr bool verbose{false};
        return ::sendPutJSONRequest(uri, mAPIAccessKey, data, verbose);
    } 
    std::shared_ptr<MLReview::Database::Connection::MongoDB>
        mMongoDBConnection{nullptr};
    std::string mAPIURL;
    std::string mAPIAccessKey;
};

/// Constructor
AcceptEventToAWS::AcceptEventToAWS(
    std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
    pImpl(std::make_unique<AcceptEventToAWSImpl> (mongoConnection))
{
}

/// Destructor
AcceptEventToAWS::~AcceptEventToAWS() = default;

/// Resource name
std::string AcceptEventToAWS::getName() const noexcept
{
    return RESOURCE_NAME;
}

/// Process request
std::unique_ptr<MLReview::Messages::IMessage> 
AcceptEventToAWS::processRequest(const nlohmann::json &request)
{
    // Figure out the query information
    auto now = std::chrono::duration_cast<std::chrono::seconds> (
        std::chrono::system_clock::now().time_since_epoch()); 
    int64_t mongoIdentifier{-1};
    if (!request.contains("identifier"))
    {   
        throw std::invalid_argument("Event identifier not set");
    }
    try
    {
        mongoIdentifier = request["identifier"].template get<int64_t> (); 
    }
    catch (...)
    {
        throw std::invalid_argument("identifier must be an int");
    }
    // Does the event exist?
    const std::string collectionName{COLLECTION_NAME};
    auto eventExists = ::checkIfEventExists(*pImpl->mMongoDBConnection,
                                            mongoIdentifier,
                                            collectionName);
    if (eventExists)
    {
        auto initialJSON = ::getParametricData(*pImpl->mMongoDBConnection,
                                               mongoIdentifier,
                                               collectionName);
        std::string authority{"UU"};
        constexpr bool isHumanReviewed{true};
        auto [isYellowstone, jsonForAPI]
             = ::createCreateUpdateRequest(initialJSON,
                                           authority,
                                           isHumanReviewed);
        // Send it
        try
        {
            if (isYellowstone)
            {
                spdlog::info("Propagating to Yellowstone endpoint");
            }
            else
            {
                spdlog::info("Propagating to Utah endpoint");
            }
            auto jsonResponse = pImpl->sendRequest(jsonForAPI, isYellowstone);
            if (jsonResponse.contains("statusCode"))
            {
                std::string message;
                if (jsonResponse.contains("message"))
                {
                    message
                        = jsonResponse["message"].template get<std::string> ();
                }
                auto statusCode
                    = jsonResponse["statusCode"].template get<int> ();
                if (statusCode >= 400 && statusCode < 500)
                {
                    spdlog::warn(
                        "Failed to accept event because of malformed request; "
                      + message);
                    auto errorResponse
                         = std::make_unique <MLReview::Messages::Error> ();
                    errorResponse->setMessage(
                        "Did not accept event at AWS because of client error");
                    errorResponse->setStatusCode(500);
                    return errorResponse;
                }
                else if (statusCode >= 500)
                {
                    spdlog::warn(
                         "Failed to accept event because of server-side error; "
                        + message);
                    auto errorResponse
                        = std::make_unique <MLReview::Messages::Error> ();
                    errorResponse->setMessage(
                        "Did not accept event at AWS because of server error");
                    errorResponse->setStatusCode(500);
                    return errorResponse;
                }
                else
                {
                    spdlog::info(
                        "Successfully submitted " 
                      + std::to_string(mongoIdentifier) 
                      + " to AWS.  Reply message from API was: "
                      + message);
                }
                // Now update MongoDB
                try
                {
                    constexpr bool submitted{true};
                    ::updateEventSubmittedInMongoDB(*pImpl->mMongoDBConnection,
                                                    mongoIdentifier,
                                                    collectionName,
                                                    submitted);
                    spdlog::info("Successfully set "
                               + std::to_string (mongoIdentifier)
                               + " to accepted in MongoDB collection "
                               + collectionName);
                                 
                }
                catch (const std::exception &e)
                {
                    spdlog::warn("Failed to update MongoDB; failed with: "
                               + std::string {e.what()});
                }
            }
            else
            {
                spdlog::warn("AWS API responded without statusCode field");
            }
        }
        catch (const std::exception &e)
        {
            spdlog::warn(e.what());
            // Return 500 error
            auto errorResponse
                = std::make_unique <MLReview::Messages::Error> ();
            errorResponse->setMessage(
                "Internal error detected when interacting with AWS REST API");
            errorResponse->setStatusCode(500); 
            return errorResponse;  
        }
    }
    else
    {
        throw std::invalid_argument("Event identifier "
                                  + std::to_string(mongoIdentifier)
                                  + " does not exist");
    }
    auto responseMessage = "Successfully propagated "
                         + std::to_string(mongoIdentifier)
                         + " to AWS and added to MongoDB";
    auto response = std::make_unique<::Response> (responseMessage); 
    return response;
}

[[nodiscard]] std::string AcceptEventToAWS::getDocumentation() const noexcept
{
    return R"""(
Accepts an event to AWS.  This additionally will update the event's event
information in the MongoDB to indicate that the event has been accepted 
as real.  To use PUT a JSON request of the form:

{"resource": "actions/acceptEventToAWS", "identifier": ml_event_identifier}

where ml_event_identifier is the integral machine learning catalog's event
identifier.
)""";
}

