#ifndef MLREVIEW_SERVICE_ACTIONS_DELETE_EVENT_FROM_AWS_HPP
#define MLREVIEW_SERVICE_ACTIONS_DELETE_EVENT_FROM_AWS_HPP
#include <memory>
#include <mlReview/service/resource.hpp>
namespace MLReview::Database::Connection
{
 class MongoDB;
}
namespace MLReview::Service::Actions
{
/// @class 
/// @brief Deletes an an event that was accepted at AWS and updates
///        the catalog.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class DeleteEventFromAWS : public MLReview::Service::IResource 
{
public:
    explicit DeleteEventFromAWS(std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoClient);

    /// @brief Destructor
    ~DeleteEventFromAWS() override;
    /// @brief Processes the user request.
    [[nodiscard]] std::unique_ptr<MLReview::Messages::IMessage> processRequest(const nlohmann::json &request) override;
    /// @result The resource's name.
    [[nodiscard]] std::string getName() const noexcept override final;
    /// @result The resource's documentation.
    [[nodiscard]] std::string getDocumentation() const noexcept override final;

    DeleteEventFromAWS(const DeleteEventFromAWS &) = delete;
    DeleteEventFromAWS& operator=(const DeleteEventFromAWS &) = delete;

private:
    class DeleteEventFromAWSImpl;
    std::unique_ptr<DeleteEventFromAWSImpl> pImpl;
};
}
#endif
