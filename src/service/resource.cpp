#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "mlReview/service/resource.hpp"
#include "mlReview/messages/message.hpp"
#include "mlReview/service/exceptions.hpp"

using namespace MLReview::Service;

/// Destructor
IResource::~IResource() = default;

/// Process a request
std::unique_ptr<MLReview::Messages::IMessage>
    IResource::processRequest(const std::string &request)
{
    auto object = nlohmann::json::parse(request);
    return processRequest(object);
}

std::unique_ptr<MLReview::Messages::IMessage>
    IResource::operator()(const std::string &request)
{
    return processRequest(request);
}

std::unique_ptr<MLReview::Messages::IMessage>
    IResource::operator()(const nlohmann::json &object)
{
    return processRequest(object);
}


/// Gets the documentation
std::string IResource::getDocumentation() const noexcept
{
   return "";
}
