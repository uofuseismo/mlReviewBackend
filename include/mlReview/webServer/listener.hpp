#ifndef MLREVIEW_WEB_SERVER_LISTENER_HPP
#define MLREVIEW_WEB_SERVER_LISTENER_HPP
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <uAuthenticator/authenticator.hpp>
namespace MLReview::Service
{
 class Handler;
}
namespace MLReview::WebServer
{
/// @class Listener "listener.hpp" "mlReview/webServer/listener.hpp"
/// @brief The listener runs the IO service, accepts incoming connections,
///        and launches sessions.  This container allows multiple threads
///        to listen on an endpoint.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Listener : public std::enable_shared_from_this<Listener>
{
public:
    /// @brief Constructor
    /// @param[in] ioContext        The ASIO input/output context.
    /// @param[in] sslContext       If creating an https session then this will
    ///                             handle the encrypted IO. 
    /// @param[in] endpoint         The URI and port on which we're listening,
    ///                             e.g., 127.0.0.1 8080.
    /// @param[in] documentRoot     The directory with the document root,
    ///                             e.g., ./
    /// @param[in] callbackHandler  This is a container of callbacks that handle
    ///                             API requests.
    /// @param[in] authenticator    A tool for authenticating users.
    Listener(boost::asio::io_context& ioContext,
             boost::asio::ssl::context &sslContext,
             boost::asio::ip::tcp::endpoint endpoint,
             const std::shared_ptr<const std::string> &documentRoot, 
             std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
             std::shared_ptr<UAuthenticator::IAuthenticator> &authenticator);
    /// @brief Begin accepting incoming connections.
    void run();
private:
    void doAccept();
    void onAccept(boost::beast::error_code errorCode,
                  boost::asio::ip::tcp::socket socket);
    boost::asio::io_context &mIOContext;
    boost::asio::ssl::context &mSSLContext;
    boost::asio::ip::tcp::acceptor mAcceptor;
    std::shared_ptr<const std::string> mDocumentRoot;
    std::shared_ptr<MLReview::Service::Handler> mCallbackHandler{nullptr};
    std::shared_ptr<UAuthenticator::IAuthenticator> mAuthenticator{nullptr};
};
}
#endif
