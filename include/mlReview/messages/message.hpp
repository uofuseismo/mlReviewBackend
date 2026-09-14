#ifndef MLREVIEW_MESSAGES_MESSAGE_HPP
#define MLREVIEW_MESSAGES_MESSAGE_HPP
#include <memory>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
namespace MLReview::Messages
{
/// @brief An abstract base class defining a message.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class IMessage
{
public:
    /// @brief Destructor.
    virtual ~IMessage();
    /// @result The data associated with this request.
    ///         By default this is null.
    [[nodiscard]] virtual std::optional<nlohmann::json> getData() const noexcept; 
    /// @result The status code associated with this request.
    [[nodiscard]] virtual int getStatusCode() const noexcept;
    /// @result True indicates the API call was a success.
    [[nodiscard]] virtual bool getSuccess() const noexcept;
    /// @result True a message accompanying the response.
    ///         By defualt this is null.
    [[nodiscard]] virtual std::optional<std::string> getMessage() const noexcept;
    /// @brief Converts the message to a binary representation.
    //[[nodiscard]] std::vector<uint8_t> toCBOR(const bool compress = false) const;
};
/// @result A serialized version of the message that will look like:
///         {
///           "message": "message details",
///           "statusCode": 200,
///           "success": true,
///           "data": {"more": "stuff"}
///         }
std::string toJSON(const std::unique_ptr<IMessage> &message, const int indent =-1);
}
#endif
