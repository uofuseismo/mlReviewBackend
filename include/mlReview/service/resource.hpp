#ifndef MLREVIEW_SERVICE_RESOURCE_HPP
#define MLREVIEW_SERVICE_RESOURCE_HPP
#include <string>
#include <nlohmann/json.hpp>
#include <mlReview/messages/message.hpp>
namespace MLReview::Service
{
/// @class IResource "resource.hpp" "drp/service/resource.hpp"
/// @brief A resource is an endpoint in the API that performs 
///        Create, Read, Update, and Delete operations.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class IResource
{
public:
    /// @brief Destructor
    virtual ~IResource();
    /// @brief Processes the user request.
    [[nodiscard]] virtual std::unique_ptr<Messages::IMessage> processRequest(const std::string &request);
    /// @brief Processes the user request.
    [[nodiscard]] virtual std::unique_ptr<Messages::IMessage> processRequest(const nlohmann::json &object) = 0;
    /// @brief Processes the user request.
    [[nodiscard]] virtual std::unique_ptr<Messages::IMessage> operator()(const std::string &request);
    /// @brief Processes the user request.
    [[nodiscard]] virtual std::unique_ptr<Messages::IMessage> operator()(const nlohmann::json &object);
    /// @result The resource's name.
    [[nodiscard]] virtual std::string getName() const noexcept = 0;
    /// @result The resource's documentation.
    [[nodiscard]] virtual std::string getDocumentation() const noexcept;
};
}
#endif
