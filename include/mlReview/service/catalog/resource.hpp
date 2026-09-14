#ifndef MLREVIEW_SERVICE_CATALOG_RESOURCE_HPP
#define MLREVIEW_SERVICE_CATALOG_RESOURCE_HPP
#include <memory>
#include <mlReview/service/resource.hpp>
namespace MLReview::Database::Connection
{
 class MongoDB;
}
namespace MLReview::Service::Catalog
{
/// @class Resource "resource.hpp" "drp/service/catalog/resource.hpp"
/// @brief The catalog resource is responsible for processing basic catalog
///        basic requests.  Effectively, this is manages the lightweight data
///        such as events, locations, arrivals, etc.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Resource : public MLReview::Service::IResource
{
public:
    explicit Resource(std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoClient);

    /// @brief Destructor
    ~Resource() override;
    /// @brief Processes the user request.
    [[nodiscard]] std::unique_ptr<MLReview::Messages::IMessage> processRequest(const nlohmann::json &request) override;
    /// @result The resource's name.
    [[nodiscard]] std::string getName() const noexcept override final;
    /// @result The resource's documentation.
    //[[nodiscard]] std::string getDocumentation() const noexcept override final;

    Resource(const Resource &) = delete;
    Resource& operator=(const Resource &) = delete;
private:
    class ResourceImpl;
    std::unique_ptr<ResourceImpl> pImpl;
};
}
#endif
