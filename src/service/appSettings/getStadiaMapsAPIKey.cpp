#include <chrono>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "mlReview/service/appSettings/getStadiaMapsAPIKey.hpp"
#include "mlReview/messages/message.hpp"
#include "mlReview/messages/error.hpp"

#define RESOURCE_NAME "actions/getStadiaMapsAPIKey"

using namespace MLReview::Service::AppSettings;

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
    /// Set the data
    void setData(const nlohmann::json &data) noexcept
    {
        auto copy = data;
        setData(std::move(copy));
    }
    void setData(nlohmann::json &&data) noexcept
    {
        mData = std::move(data); 
        mHaveData = true;
    }
    /// Get the data
    std::optional<nlohmann::json> getData() const noexcept
    {
        if (mHaveData)
        {
            return std::optional<nlohmann::json> (mData);
        }
        return std::nullopt;
    }

    ~Response() final = default;
    std::string mMessage;
    nlohmann::json mData;
    bool mHaveData;
};

}


class GetStadiaMapsAPIKey::GetStadiaMapsAPIKeyImpl
{
public:
    GetStadiaMapsAPIKeyImpl(const std::string &apiKey)
    {
        if (apiKey.empty()){throw std::runtime_error("API key is empty");}
        mAPIKey = apiKey;
    }
    std::string mAPIKey;
};

/// Constructor
GetStadiaMapsAPIKey::GetStadiaMapsAPIKey(const std::string &apiKey) :
    pImpl(std::make_unique<GetStadiaMapsAPIKeyImpl> (apiKey))
{
}

/// Destructor
GetStadiaMapsAPIKey::~GetStadiaMapsAPIKey() = default;

/// Resource name
std::string GetStadiaMapsAPIKey::getName() const noexcept
{
    return RESOURCE_NAME;
}

/// Process request
std::unique_ptr<MLReview::Messages::IMessage> 
GetStadiaMapsAPIKey::processRequest(const nlohmann::json &request)
{
    // Figure out the query information
    auto now = std::chrono::duration_cast<std::chrono::seconds> (
        std::chrono::system_clock::now().time_since_epoch()); 
    auto responseMessage = "Returning Stadia Maps API key";
    auto response = std::make_unique<::Response> (responseMessage); 
    nlohmann::json object;
    object["key"] = pImpl->mAPIKey;
    response->setData(object);
    return response;
}

[[nodiscard]] std::string GetStadiaMapsAPIKey::getDocumentation() const noexcept
{
    return R"""(
Gets the Stadia maps API key.

{"resource": "appSettings/getStadiaMapsAPIKey"}

)""";
}

