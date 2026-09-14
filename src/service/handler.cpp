#include <optional>
#include <vector>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "mlReview/service/handler.hpp"
#include "mlReview/service/resource.hpp"
#include "mlReview/messages/error.hpp"

using namespace MLReview::Service;

namespace
{
class ResourcesMessage : public MLReview::Messages::IMessage
{
public:
    explicit ResourcesMessage(std::vector<std::string> &&resources) :
        mResources(std::move(resources))
    {
    }
    ~ResourcesMessage() override = default;
    std::optional<nlohmann::json> getData() const noexcept override final
    {
        if (mResources.empty()){return nullptr;}
        nlohmann::json data;
        data["resources"] = mResources;
        return std::optional<nlohmann::json> (std::move(data));
    };
    std::optional<std::string> getMessage() const noexcept override final
    {
        std::string message{"successfully returned available resources"};
        if (mResources.empty())
        {
            message = message + "; however there are no resources";
        }
        return std::optional<std::string> (message);
    } 
    /*
    std::unique_ptr<MLReview::Messages::IMessage> clone() const
    {
        auto message = std::make_unique<::ResourcesMessage> (*this);
        return message;
    } 
    */
    int getStatusCode() const noexcept override final {return 200;};
    bool getSuccess() const noexcept override final {return true;}; 
    std::vector<std::string> mResources;
};
}

class Handler::HandlerImpl
{
public:
    std::map<std::string, std::unique_ptr<IResource>> mResources;
};

/// Constructor
Handler::Handler() :
    pImpl(std::make_unique<HandlerImpl> ())
{
}

/// Move constructor
Handler::Handler(Handler &&handler) noexcept
{
    *this = std::move(handler);
}

/// Move operator
Handler& Handler::operator=(Handler &&handler) noexcept
{
   if (&handler == this){return *this;}
   pImpl = std::move(handler.pImpl);
   return *this;
}

/// Destructor
Handler::~Handler() = default;

/// Insert
void Handler::insert(std::unique_ptr<IResource> &&resource)
{
    if (resource == nullptr){throw std::invalid_argument("Resource is NULL");}
    auto name = resource->getName();
    if (pImpl->mResources.contains(name))
    {
        throw std::invalid_argument("Resource " + name + " already exists");
    }
    pImpl->mResources.insert(std::pair {name, std::move(resource)});
}

std::vector<std::string> Handler::getResources() const noexcept
{
    std::vector<std::string> result;
    result.reserve(pImpl->mResources.size());
    for (const auto &resource : pImpl->mResources)
    {
        result.push_back(resource.first);
    }
    return result;
}

/// Processes a message
std::unique_ptr<MLReview::Messages::IMessage> 
Handler::process(const std::string &request) const
{
    // Empty message
    if (request.empty())
    {
        auto response = std::make_unique <MLReview::Messages::Error> (); 
        response->setStatusCode(400);
        response->setMessage("request is empty");
        return response;
    }

    // Parse the message
    try
    {
        auto object = nlohmann::json::parse(request);
        if (!object.contains("resource"))
        {
             throw std::invalid_argument("resource not specified");
        }
        // API query
        auto resourceName = object["resource"].template get<std::string> ();
        if (resourceName == "resources")
        {
            auto resources = getResources();
            auto response
                = std::make_unique<::ResourcesMessage>
                  (std::move(resources));
            return response;
        }
        // Otherwise this wants a resource 
        auto resource = pImpl->mResources.find(resourceName);
        if (resource == pImpl->mResources.end())
        {
            auto response = std::make_unique <MLReview::Messages::Error> ();
            response->setStatusCode(400);
            response->setMessage("resource: " + resourceName
                               + " does not exist");
            return response;
        }
        else
        {
            return resource->second->processRequest(object);
        }
    }
    catch (const std::runtime_error &e)
    {
        auto response = std::make_unique <MLReview::Messages::Error> ();
        response->setStatusCode(500);
        response->setMessage("Internal server error");
        return response;
    }
    catch (const std::invalid_argument &e)
    {
        auto response = std::make_unique <MLReview::Messages::Error> (); 
        response->setStatusCode(400);
        response->setMessage(std::string {e.what()});
        return response;
    } 
/*
    try
    {
         
    }
    catch (const std::exception &e)
    {
        spdlog::warn(e.what()); 
*/
        auto response = std::make_unique <MLReview::Messages::Error> ();
        response->setStatusCode(500);
//        response->setMessage(std::string {e.what()});
        return response;
/*
    }
*/
}
