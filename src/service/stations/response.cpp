#include <string>
#include <nlohmann/json.hpp>
#include "mlReview/service/stations/response.hpp"

using namespace MLReview::Service::Stations;

class Response::ResponseImpl
{
public:
    nlohmann::json mData;
    std::string mMessage;
};

/// Constructor
Response::Response() :
    pImpl(std::make_unique<ResponseImpl> ()) 
{
}

/// Destructor
Response::~Response() = default;

/// Accompanying message details
void Response::setMessage(const std::string &details) noexcept
{
    pImpl->mMessage = details;
}

std::optional<std::string> Response::getMessage() const noexcept
{
    return !pImpl->mMessage.empty() ?
           std::optional<std::string> (pImpl->mMessage) : std::nullopt;
}

/// Set the data
void Response::setData(const nlohmann::json &data) noexcept
{
    auto copy = data;
    setData(std::move(copy));
}

void Response::setData(nlohmann::json &&data) noexcept
{
    pImpl->mData = std::move(data); 
}

/// Get the data
std::optional<nlohmann::json> Response::getData() const noexcept
{
    if (!pImpl->mData.empty())
    {
        return std::optional<nlohmann::json> (pImpl->mData);
    }
    return std::nullopt;
}
