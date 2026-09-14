#ifndef MLREVIEW_SERVICE_ACTIONS_ACCEPT_EVENT_TO_AWS_HPP
#define MLREVIEW_SERVICE_ACTIONS_ACCEPT_EVENT_TO_AWS_HPP
#include <memory>
#include <mlReview/service/resource.hpp>
namespace MLReview::Database::Connection
{
 class MongoDB;
}
namespace MLReview::Service::Actions
{
/// @class 
/// @brief Accepts an event and propagetes its details to the machine
///        learning catalog hosted at AWS.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class AcceptEventToAWS : public MLReview::Service::IResource 
{
public:
    explicit AcceptEventToAWS(std::shared_ptr<MLReview::Database::Connection::MongoDB> &mongoClient);

    /// @brief Destructor
    ~AcceptEventToAWS() override;
    /// @brief Processes the user request.
    [[nodiscard]] std::unique_ptr<MLReview::Messages::IMessage> processRequest(const nlohmann::json &request) override;
    /// @result The resource's name.
    [[nodiscard]] std::string getName() const noexcept override final;
    /// @result The resource's documentation.
    [[nodiscard]] std::string getDocumentation() const noexcept override final;

    AcceptEventToAWS(const AcceptEventToAWS &) = delete;
    AcceptEventToAWS& operator=(const AcceptEventToAWS &) = delete;

private:
    class AcceptEventToAWSImpl;
    std::unique_ptr<AcceptEventToAWSImpl> pImpl;
};
}
#endif
