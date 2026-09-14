#ifndef MLREVIEW_MESSAGES_AUTHORIZED_HPP
#define MLREVIEW_MESSAGES_AUTHORIZED_HPP
#include <memory>
#include <optional>
#include <mlReview/messages/message.hpp>
namespace MLReview::Messages
{
/// @class Authorized "authorized.hpp" "mlReview/messages/authorized.hpp"
/// @brief A one-off authorized response message when not using the websocket but simple HTTP.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Authorized final : public IMessage
{
public:
    /// @brief Constructor.
    Authorized() = default;
    explicit Authorized(const std::string &jwt)
    {
        setJSONWebToken(jwt);
    }
    /// @brief Copy constructor.
    Authorized(const Authorized &message) = default;
    /// @brief Move constructor.
    Authorized(Authorized &&authorized) noexcept = default;
    /// @result The error code.
    [[nodiscard]] int getStatusCode() const noexcept override final
    {
        return 200;
    }

    /// @brief Sets the details of the error message. 
    /// @param[in] details  The details of the error message. 
    void setJSONWebToken(const std::string &jwt)
    {
        if (jwt.empty()){throw std::invalid_argument("Web token is empty");}
        mData["jsonWebToken"] = jwt; 
    }
    /// @result The details of the error message.
    [[nodiscard]] std::optional<std::string> getMessage() const noexcept final
    {
        return "Successfully logged into mlReview API";
    }

    /// @result Flag indicating the request was successful.
    [[nodiscard]] bool getSuccess() const noexcept override final
    { 
        return true;
    }

    /*
    void setData(const nlohmann::json &data) noexcept
    {
        auto copy = data;
        setData(std::move(copy));
    }

    void setData(nlohmann::json &&data) noexcept
    {
        mData = std::move(data); 
    }
    */

    std::optional<nlohmann::json> getData() const noexcept override final
    {
        return std::optional<nlohmann::json> (mData);
    }

    /// @brief Destructor.
    ~Authorized() override = default;

    /// @brief Clones this class.
    std::unique_ptr<IMessage> clone() const
    {
        std::unique_ptr<MLReview::Messages::IMessage> result
            = std::make_unique<Authorized> (*this);
        return result;
    }

    Authorized& operator=(const Authorized &message)
    {
        mData = message.mData;
        return *this;
    }
    Authorized& operator=(Authorized &&message) noexcept
    {
        mData = std::move(message.mData);
        return *this;
    }
private:
    nlohmann::json mData;
};
}
#endif
