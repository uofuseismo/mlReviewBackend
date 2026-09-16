#ifndef MLREVIEW_SERVICE_ACTIONS_DELETE_EVENT_FROM_AWS_HPP
#define MLREVIEW_SERVICE_ACTIONS_DELETE_EVENT_FROM_AWS_HPP
#include <memory>
#include <string>
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
    /// @brief Constructor.
    /// @param[in] mongoClient   The MongoDB connection.
    /// @param[in] apiURL        The URL of the machine learning catalog API
    ///                          hosted at AWS.
    /// @param[in] apiAccessKey  The access key for that API.
    /// @throws std::invalid_argument if the connection is NULL, or the URL
    ///         or access key is empty.
    DeleteEventFromAWS(std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoClient,
                       const std::string &apiURL,
                       const std::string &apiAccessKey);

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
