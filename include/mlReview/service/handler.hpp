#ifndef MLREVIEW_SERVICE_HANDLER_HPP
#define MLREVIEW_SERVICE_HANDLER_HPP
#include <memory>
#include <mlReview/messages/message.hpp>
namespace MLReview::Service
{
 class IResource;
}
namespace MLReview::Service
{
/// @class The request handler processes API requests.  This is a form of
///        two-way communication in which the client asks and the server
///        provides.  Individual resources handle the CRUD operations.
class Handler
{
public:
    /// @brief Constructor.
    Handler();
    /// @brief Move constructor.
    /// @param[in,out] handler  The handler from which to initialize this class.
    ///                         On exit, handler's behavior is undefined.
    Handler(Handler &&handler) noexcept;

    /// @brief Processes a request.
    /// @param[in] request  The input request message - this can be binary
    ///                     (e.g., CBOR) or string data (e.g., JSON).
    [[nodiscard]] std::unique_ptr<MLReview::Messages::IMessage> process(const std::string &request) const;

    /// @brief Inserts a resource to the handler.
    void insert(std::unique_ptr<IResource > &&resource);

    [[nodiscard]] std::vector<std::string> getResources() const noexcept;

    /// @brief Destructor.
    ~Handler();

    /// @brief Move assignment operator.
    /// @param[in,out] handler  The handler whose memory will be moved to this.
    ///                         On exit, handler's behavior is undefined.
    /// @result The memory from handler moved to this.
    Handler& operator=(Handler &&handler) noexcept;

    Handler(const Handler &) = delete;
    Handler& operator=(Handler &) = delete;
private:
    class HandlerImpl;
    std::unique_ptr<HandlerImpl> pImpl;
};
}
#endif

