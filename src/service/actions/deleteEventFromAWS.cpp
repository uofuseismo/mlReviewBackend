#include <string>
#include "mlReview/service/actions/deleteEventFromAWS.hpp"
#include "mlReview/messages/error.hpp"
#include "curl.hpp"
#include "mongoUtilities.hpp"

#define RESOURCE_NAME "actions/deleteEventFromAWS"

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

[[nodiscard]]
std::pair<bool, nlohmann::json>
   createDeleteRequest(const nlohmann::json &initialResponse)
{
   nlohmann::json result;
   auto eventIdentifier = initialResponse["eventIdentifier"].template get<int64_t> ();
   auto parametricData = initialResponse["parametricData"];
   auto preferredOrigin = parametricData["preferredOrigin"];
   result["identifier"] = "urts" + std::to_string(eventIdentifier);
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

class DeleteEventFromAWS::DeleteEventFromAWSImpl
{
public:
    DeleteEventFromAWSImpl(
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
        return ::sendDeleteJSONRequest(uri, mAPIAccessKey, data, verbose);
    }
    std::shared_ptr<MLReview::Database::Connection::MongoDB>
        mMongoDBConnection{nullptr};
    std::string mAPIURL;
    std::string mAPIAccessKey;
};

/// Constructor
DeleteEventFromAWS::DeleteEventFromAWS(
    std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoConnection) :
    pImpl(std::make_unique<DeleteEventFromAWSImpl> (mongoConnection))
{
}

/// Destructor
DeleteEventFromAWS::~DeleteEventFromAWS() = default;

/// Resource name
std::string DeleteEventFromAWS::getName() const noexcept
{
    return RESOURCE_NAME;
}

[[nodiscard]] std::string DeleteEventFromAWS::getDocumentation() const noexcept
{
    return R"""(
Removes an event from AWS.  This additionally will update the event's event
information in the MongoDB to indicate that the event has been removed from
AWS.  To use PUT a JSON request of the form:

{"resource": "actions/deleteEventFromAWS", "identifier": ml_event_identifier}

where ml_event_identifier is the integral machine learning catalog's event
identifier.
)""";
}

/// Process request
std::unique_ptr<MLReview::Messages::IMessage> 
DeleteEventFromAWS::processRequest(const nlohmann::json &request)
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
        throw std::invalid_argument("idnetifier must be an int");
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
        auto [isYellowstone, jsonForAPI]
           = createDeleteRequest(initialJSON);
        // Send it
        try
        {
            if (isYellowstone)
            {
                spdlog::info("Propagating delete to Yellowstone endpoint");
            }
            else
            {
                spdlog::info("Propagating delete to Utah endpoint");
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
                        "Failed to delete event because of malformed request; "
                      + message);
                    auto errorResponse
                         = std::make_unique <MLReview::Messages::Error> (); 
                    errorResponse->setMessage(
                        "Did not delete event at AWS because of client error");
                    errorResponse->setStatusCode(500);
                    return errorResponse;
                }
                else if (statusCode >= 500)
                {
                    spdlog::warn(
                         "Failed to delete event because of server-side error; "
                        + message);
                    auto errorResponse
                        = std::make_unique <MLReview::Messages::Error> ();
                    errorResponse->setMessage(
                        "Did not delete event at AWS because of server error");
                    errorResponse->setStatusCode(500);
                    return errorResponse;
                }
                else
                {
                    spdlog::info(
                        "Successfully deleted " 
                      + std::to_string(mongoIdentifier) 
                      + " from AWS.  Reply message from API was: "
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
    auto responseMessage = "Successfully deleted "
                         + std::to_string(mongoIdentifier)
                         + " from AWS and downgraded in MongoDB";
    auto response = std::make_unique<::Response> (responseMessage); 
    return response;

}
