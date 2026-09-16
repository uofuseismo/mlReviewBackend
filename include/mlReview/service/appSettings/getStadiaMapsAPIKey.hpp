#ifndef MLREVIEW_SERVICE_APP_SETTINGS_GET_STADIA_MAPS_KEY_HPP
#define MLREVIEW_SERVICE_APP_SETTINGS_GET_STADIA_MAPS_KEY_HPP
#include <memory>
#include <string>
#include <mlReview/service/resource.hpp>
namespace MLReview::Service::AppSettings
{
/// @class GetStadiaMapsAPIKey
/// @brief Gets the stadia maps key.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class GetStadiaMapsAPIKey : public MLReview::Service::IResource 
{
public:
    /// @brief Constructor.
    /// @param[in] key   The Stadia Maps API key.
    /// @throws std::invalid_argument if key is empty.
    explicit GetStadiaMapsAPIKey(const std::string &key);

    /// @brief Destructor
    ~GetStadiaMapsAPIKey() override;
    /// @brief Processes the user request.
    [[nodiscard]] std::unique_ptr<MLReview::Messages::IMessage> processRequest(const nlohmann::json &request) override;
    /// @result The resource's name.
    [[nodiscard]] std::string getName() const noexcept override final;
    /// @result The resource's documentation.
    [[nodiscard]] std::string getDocumentation() const noexcept override final;

    GetStadiaMapsAPIKey(const GetStadiaMapsAPIKey &) = delete;
    GetStadiaMapsAPIKey& operator=(const GetStadiaMapsAPIKey &) = delete;

private:
    class GetStadiaMapsAPIKeyImpl;
    std::unique_ptr<GetStadiaMapsAPIKeyImpl> pImpl;
};
}
#endif
