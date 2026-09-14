#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <queue>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/config.hpp>
#include <boost/optional.hpp>
#include <spdlog/spdlog.h>
#include <uAuthenticator/authenticator.hpp>
#include <uAuthenticator/credentials.hpp>
#include "mlReview/service/handler.hpp"
#include "mlReview/messages/message.hpp"
#include "mlReview/messages/error.hpp"
#include "responses.hpp"
#include "authorized.hpp"
#include "base64.hpp"
#include <boost/algorithm/string.hpp>

#define SERVICE_NAME "UUSS ML Review"

namespace
{

// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
boost::beast::http::message_generator
handleRequest(
    const std::string &jsonWebToken,
    boost::beast::string_view documentRoot,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request,
    std::shared_ptr<MLReview::Service::Handler> &callbackHandler)
{

    const auto successResponse = [&request](const std::string &payload)
    {
        spdlog::info("Success: Message response size: "
                   + std::to_string (payload.size()));
        boost::beast::http::response<boost::beast::http::string_body> result
        {    
            boost::beast::http::status::ok,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = payload;
        result.prepare_payload();
        return result;
    };

    const auto badRequest = [&request](boost::beast::string_view why)
    {
        spdlog::info("Bad request");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::bad_request,
            request.version()
        };  
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\", \"reason\":\""
                      + std::string(why)
                      + "\"}";
        result.prepare_payload();
        return result;
    };  

    // Returns a server error response
    const auto serverError = [&request](boost::beast::string_view what)
    {
        spdlog::info("Server error");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::internal_server_error,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\",\"reason\":\""
                      + std::string(what)
                      + "\"}";
        result.prepare_payload();
        return result;
    };


    /// Options request for CORS
    const auto optionsHandler = [&request]()
    {
        spdlog::info("CORS");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::no_content,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set("Access-Control-Allow-Credentials",
                   "true");
        result.set(boost::beast::http::field::access_control_allow_methods,
                   "GET,HEAD,OPTIONS,POST,PUT");
        result.set(boost::beast::http::field::access_control_allow_headers,
                   "Access-Control-Allow-Origin, Access-Control-Allow-Headers, Access-Control-Allow-Methods, Connection, Origin, Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers, Authorization");
        result.set(boost::beast::http::field::access_control_max_age,
                   "3600");
        result.set(boost::beast::http::field::connection, //"Connection",
                   "close");
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "text/html");
        result.keep_alive(request.keep_alive());
        result.body() = "";
        result.prepare_payload();
        return result;
    };

    // This comes up during CORS weirdness.  Basically, we need to tell
    // the browser all the headers the backend will accept.
    if (request.method() == boost::beast::http::verb::options)
    {   
        return optionsHandler();
    }   

    // Make sure we can handle the method
    if (request.method() != boost::beast::http::verb::get  &&
        request.method() != boost::beast::http::verb::put  &&  
        request.method() != boost::beast::http::verb::post)
    {   
        return badRequest("Unknown HTTP-method");
    }   

    // Attempt to respond to request with the callback handler 
    try
    {
        std::string requestMessage = request.body();
        // Could be a straight login
        if (requestMessage.empty())
        {
            std::unique_ptr<MLReview::Messages::IMessage> responseMessage
                 = std::make_unique<MLReview::Messages::Authorized> (jsonWebToken);
            return successResponse(MLReview::Messages::toJSON(responseMessage));
        }
        // Otherwise process
        auto responseMessage = callbackHandler->process(requestMessage);
        if (responseMessage)
        {
            return successResponse(MLReview::Messages::toJSON(responseMessage));
        }
        else
        {
            return serverError("Callback returned a NULL message");
        }
    }
    catch (const std::exception &e)
    {
        spdlog::warn("::handleRequest reply failed with "
                   + std::string{e.what()});
        try
        {
            return serverError("server error - unhandled exception");
        }
        catch (const std::exception &e)
        {
            spdlog::error("::handleRequest error failed with "
                        + std::string {e.what()});
        }
    }

    return serverError("Unhandled method");
}

///--------------------------------------------------------------------------///
///                               Web Sockets                                ///
///--------------------------------------------------------------------------///
// Echoes back all received WebSocket messages.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template<class Derived>
class WebSocketSession
{
public:
    // Start the asynchronous operation
    template<class Body, class Allocator>
    void run(boost::beast::http::request
             <
              Body,
              boost::beast::http::basic_fields<Allocator>
             > request)
    {
        // Accept the WebSocket upgrade request
        doAccept(std::move(request));
    }
private:
    // Access the derived class, this is part of
    // the Curiously Recurring Template Pattern idiom.
    Derived& derived()
    {
        return static_cast<Derived &> (*this);
    }

    // Start the asynchronous operation
    template<class Body, class Allocator>
    void doAccept(boost::beast::http::request
                  <
                   Body,
                   boost::beast:: http::basic_fields<Allocator>
                  > request)
    {
        // Set suggested timeout settings for the websocket
        derived().ws().set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        derived().ws().set_option(
            boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type &result)
            {
                result.set(boost::beast::http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING)
                  + " " + SERVICE_NAME);
            }));

        // Enable deflation on response messages.  Requests are small.
        boost::beast::websocket::permessage_deflate deflateMessageOption;
        //deflateMessageOption.msg_size_threshold = 0; // Bytes (0 is default)
        deflateMessageOption.client_enable = false; // Clients
        deflateMessageOption.server_enable = true;  // Servers (me!)
        derived().ws().set_option(deflateMessageOption);

        // Accept the websocket handshake
        derived().ws().async_accept(
            request,
            boost::beast::bind_front_handler(
                &WebSocketSession::onAccept,
                derived().shared_from_this()));
    }

    void onAccept(boost::beast::error_code errorCode)
    {
        if (errorCode)
        {
            spdlog::critical("WebSocketSession::onAccept failed with "
                           + std::string {errorCode.what()});
            return;
        }

        // Read a message
        doRead();
    }

    void onRead(
        boost::beast::error_code errorCode,
        const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This indicates that the websocket_session was closed
        if (errorCode == boost::beast::websocket::error::closed)
        {
            spdlog::info("WebSocketSession::onRead socket closed");
            return;
        }

        if (errorCode)
        {
            spdlog::warn("WebSocketSession::onRead failed with "
                       + std::string {errorCode.what()});
            return;
        }

        // Get the request message
        derived().ws().text(derived().ws().got_text());
        auto requestMessage
             = boost::beast::buffers_to_string(mReadBuffer.data());
        mReadBuffer.consume(mReadBuffer.size());
        
        // Attempt to do something with the thread
        try
        {
            auto responseMessage
                = derived().getCallbackHandler()->process(requestMessage);
            if (responseMessage)
            {
                reply(MLReview::Messages::toJSON(responseMessage));
            }
            else
            {
                throw std::runtime_error("Callback returned a NULL message");
            }
/*
            derived().ws().async_write(
                mReadBuffer.data(),
                boost::beast::bind_front_handler(
                    &WebSocketSession::onWrite,
                    derived().shared_from_this()));
*/
        }
        catch (const std::exception &e)
        {
            spdlog::warn("WebSocketSession::onRead reply failed with "
                       + std::string{e.what()});
            try
            {
                MLReview::Messages::Error errorMessage;
                errorMessage.setStatusCode(500);
                errorMessage.setMessage("server error - unhandled exception");
                reply(MLReview::Messages::toJSON(errorMessage.clone()));
            }
            catch (const std::exception &e)
            {
                spdlog::error("WebSocketSession::onRead error failed with "
                            + std::string {e.what()});
            }
        }

        // Go back to reading 
        derived().ws().async_read(
            mReadBuffer,
            boost::beast::bind_front_handler(
                &WebSocketSession::onRead,
                derived().shared_from_this()));
    }

    void reply(const std::string &responseString)
    {
        reply(std::make_shared<std::string> (responseString));
    }

    void reply(const std::shared_ptr<const std::string> &stringStream)
    {
        // Post our work to the strand, this ensures that the members of `this'
        // will not be accessed concurrently.
        boost::asio::post
        (
            derived().ws().get_executor(),
            boost::beast::bind_front_handler
            (
                &WebSocketSession::queueSend,
                derived().shared_from_this(),
                stringStream
            )
        );

        // Fall through to on read  
    }

    void queueSend(const std::shared_ptr<const std::string> &response)
    {
        // Allocate and store the message
        mResponseQueue.push(std::move(response));

        // Are we already writing?
        if (mResponseQueue.size() > 1)
        {
            spdlog::debug("WebSocketSession: queueSend is already writing...");
            return;
        }

        // We are not currently writing so send this message
        if (!mResponseQueue.empty())
        {
            derived().ws().async_write(
                boost::asio::buffer(std::move(*mResponseQueue.front())),
                boost::beast::bind_front_handler(
                   &WebSocketSession::onWrite,
                   derived().shared_from_this()));
        }

        // Fall through to reply 
    }

    void onWrite(boost::beast::error_code errorCode,
                 const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (errorCode)
        {
            spdlog::critical("WebSocketSession::onWrite failed with "
                           + std::string {errorCode.what()});
            return;
        }

        // Remove the latest sent message from the response queue
        mResponseQueue.pop();
 
        // Send the next message if possible
        if (!mResponseQueue.empty()) 
        {
            derived().ws().async_write(
                boost::asio::buffer(std::move(*mResponseQueue.front())),
                boost::beast::bind_front_handler(
                   &WebSocketSession::onWrite,
                   derived().shared_from_this()));
        }

        // Fall through to queueSend 
    }

    void doRead()
    {
        // Read a message into our buffer
        derived().ws().async_read(
            mReadBuffer,
            boost::beast::bind_front_handler(
                &WebSocketSession::onRead,
                derived().shared_from_this()));
    }

    boost::beast::flat_buffer mReadBuffer;
    boost::beast::flat_buffer mWriteBuffer;
    std::queue<std::shared_ptr<const std::string>> mResponseQueue;
    bool mWriting{false};
};

// Handles a plain WebSocket connection
class PlainWebSocketSession :
    public ::WebSocketSession<::PlainWebSocketSession>,
    public std::enable_shared_from_this<::PlainWebSocketSession>
{
public:
    // Create the session
    PlainWebSocketSession(
        const std::string sessionIdentifier, 
        boost::beast::tcp_stream &&stream,
        std::shared_ptr<MLReview::Service::Handler> &callbackHandler) :
        mWebSocket(std::move(stream)),
        mCallbackHandler(callbackHandler),
        mSessionIdentifier(sessionIdentifier)
    {
        //mWebSocket.binary(true);
    }

    ~PlainWebSocketSession()
    {
        spdlog::info("PlainWebSocketSession: Terminating session for "
                   + mSessionIdentifier);
    }       

    // Called by the base class to get a handle to the websocket
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws()
    {
        return mWebSocket;
    }

    // Called by the base class to get a handle to the callback handler
    std::shared_ptr<MLReview::Service::Handler> getCallbackHandler()
    {
        return mCallbackHandler;
    }
private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> mWebSocket;
    std::shared_ptr<MLReview::Service::Handler> mCallbackHandler;
    std::string mSessionIdentifier;
};

// Handles an SSL WebSocket connection
class SSLWebSocketSession :
    public ::WebSocketSession<::SSLWebSocketSession>,
    public std::enable_shared_from_this<::SSLWebSocketSession>
{
public:
    // Create the ssl_websocket_session
    SSLWebSocketSession(
        const std::string &sessionIdentifier,
        boost::asio::ssl::stream<boost::beast::tcp_stream> &&stream,
        std::shared_ptr<MLReview::Service::Handler> &callbackHandler) :
        mWebSocket(std::move(stream)),
        mCallbackHandler(callbackHandler),
        mSessionIdentifier(sessionIdentifier)
    {
        //mWebSocket.binary(true);
    }

    ~SSLWebSocketSession()
    {
        spdlog::info("SSLWebSocketSession: Terminating session for: " 
                   + mSessionIdentifier);
    }

    // Called by the base class
    boost::beast::websocket::stream
    <
     boost::asio::ssl::stream<boost::beast::tcp_stream>
    > &ws()
    {
        return mWebSocket;
    }

    std::shared_ptr<MLReview::Service::Handler> getCallbackHandler()
    {
        return mCallbackHandler;
    }
private:
    boost::beast::websocket::stream
    <
     boost::asio::ssl::stream<boost::beast::tcp_stream>
    > mWebSocket;
    std::shared_ptr<MLReview::Service::Handler> mCallbackHandler;
    std::string mSessionIdentifier;
};


/// Convenience function to make and run a websocket session
template<class Body, class Allocator>
void makeWebSocketSession(
    const std::string sessionIdentifier,
    boost::beast::tcp_stream stream,
    std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
    boost::beast::http::request
    <
     Body,
     boost::beast::http::basic_fields<Allocator>
    > request)
{
    std::make_shared<::PlainWebSocketSession>(
        sessionIdentifier, std::move(stream), callbackHandler
    )->run(std::move(request));
}

template<class Body, class Allocator>
void makeWebSocketSession(
    const std::string sessionIdentifier,
    boost::asio::ssl::stream<boost::beast::tcp_stream> stream,
    std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
    boost::beast::http::request
    <
     Body,
     boost::beast::http::basic_fields<Allocator>
    > request)
{
    std::make_shared<SSLWebSocketSession>(
        sessionIdentifier, std::move(stream), callbackHandler
    )->run(std::move(request));
}

///--------------------------------------------------------------------------///
///                               HTTP Session                               ///
///--------------------------------------------------------------------------///
// Handles an HTTP server connection.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template<class Derived>
class HTTPSession
{
public:
    HTTPSession(boost::beast::flat_buffer buffer,
                const std::shared_ptr<const std::string> &documentRoot,
                std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
                std::shared_ptr<UAuthenticator::IAuthenticator> &authenticator) :
        mBuffer(std::move(buffer)),
        mDocumentRoot(documentRoot),
        mCallbackHandler(callbackHandler),
        mAuthenticator(authenticator)
    {
    }

    void doRead()
    {
        // Construct a new parser for each message
        mParser.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        mParser->body_limit(mMaximumMessageSizeInBytes);

        // Set the timeout.
        boost::beast::get_lowest_layer(
            derived().stream()).expires_after(mTimeOut);

        // Read a request using the parser-oriented interface
        boost::beast::http::async_read(
            derived().stream(),
            mBuffer,
            *mParser,
            boost::beast::bind_front_handler(
                &HTTPSession::onRead,
                derived().shared_from_this()));
    }

    void onRead(boost::beast::error_code errorCode,
                const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This means they closed the connection
        if (errorCode == boost::beast::http::error::end_of_stream)
        {
            return derived().closeConnection();
        }

        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical("HTTPSession::onRead read failed with "
                               + std::string {errorCode.what()});
            }
            return;
        }

        // Handle CORS
        if (mParser->get().method() == boost::beast::http::verb::options)
        {
            return queueWrite(::createCORSResponse(mParser->release()));
        }

        std::string sessionIdentifier;
        bool authenticated{false};
        auto header = mParser->get().base();
 
        std::string jsonWebToken;
        auto authorizationIndex = header.find("Authorization");
        if (authorizationIndex != header.end())
        {
            try
            {
                std::vector<std::string> authorizationField;
                boost::split(authorizationField,
                             header["Authorization"],
                             boost::is_any_of(" \t\n"));
                if (!authorizationField.empty())
                {
                    auto authorizationType = authorizationField.at(0);
                    boost::algorithm::trim(authorizationType);
                    std::transform(authorizationType.begin(),
                                   authorizationType.end(),
                                   authorizationType.begin(),
                                   ::toupper);
                   if (authorizationType == "BASIC")
                   {
                       auto hexCredentials = authorizationField.at(1);
                       boost::algorithm::trim(hexCredentials);
                       auto credentials = base64::from_base64(hexCredentials);
                       auto splitIndex = credentials.find(":");
                       if (splitIndex != credentials.npos)
                       {
                           std::string userName
                               = credentials.substr(0, splitIndex);
                           sessionIdentifier = userName;
                           std::string password;
                           if (splitIndex < credentials.length() - 1)
                           {
                               password
                                   = credentials.substr(splitIndex + 1,
                                                        credentials.length());
                               authenticated = true;
                               constexpr UAuthenticator::Permissions
                                   permissions{UAuthenticator::
                                               Permissions::ReadWrite};
                               if (mAuthenticator)
                               {
                                   auto authenticationStatus
                                       = mAuthenticator->authenticate(userName,
                                                                      password,
                                                                      permissions);
                                   if (authenticationStatus ==
                                       UAuthenticator::IAuthenticator::
                                       ReturnCode::Allowed)
                                   {
                                       spdlog::info("Allowing " + userName);
                                       auto credentials
                                           = mAuthenticator->getCredentials(userName);
                                       if (credentials)
                                       {
                                           auto jwt = credentials->getToken(); 
                                           if (jwt){jsonWebToken = *jwt;}
                                       }
                                   }
                                   else if (authenticationStatus ==
                                            UAuthenticator::IAuthenticator::
                                            ReturnCode::Denied)
                                   {
                                       spdlog::warn("Blocking " + userName);
                                       authenticated = false;
                                   }
                                   else
                                   {
                                       spdlog::critical(
                                          "Internal authentication error for user " + userName);
                                       return queueWrite(::createInternalServerErrorResponse(
                                          "Server error - authentication failed",
                                           mParser->release()));
                                   }
                               }
                               else
                               {
                                   spdlog::info("No authentication; allowing: "
                                              + userName);//  + " " + password);
                               }
                           }
                       }
                   }
                   else if (authorizationType == "BEARER")
                   {
                       authenticated = false;
                       jsonWebToken = authorizationField.at(1);
                       if (mAuthenticator)
                       {
                           auto credentials = mAuthenticator->authorize(jsonWebToken);
                           if (credentials)
                           {
                               auto userName = credentials->getUser();
                               if (userName)
                               {
                                   spdlog::info("Authorizing " + *userName);
                               }
                               else
                               {
                                   spdlog::info("Authorizing unknown user");
                               }
                               authenticated = true;
                           }
                           else
                           {
                               spdlog::info("Authorization failed");
                               authenticated = false;
                           } 
                       }
                       else
                       {
                           spdlog::info("No authentication; authorizing");
                       }
                   }
                }
            }
            catch (const std::exception &e)
            {
                spdlog::error("No credentials supplied");
            }
        }
         

        if (boost::beast::websocket::is_upgrade(mParser->get()))
        {
            spdlog::info("WS upgrade request");
            // Smuggle the key in from the Sec-WebSocket-Protocol.
            // Note, this is not a standard practice so the front end
            // needs to know this hack.
            auto authorizationIndex = header.find("Sec-WebSocket-Protocol");
            if (authorizationIndex != header.end())
            {
                if (mAuthenticator)
                {
                    auto key = std::string {header["Sec-WebSocket-Protocol"]};
                    if (!key.empty())
                    {
                        try
                        {
                            spdlog::info("Validating " + key);
                            auto credentials = mAuthenticator->authorize(key);
                            if (credentials)
                            {
                                auto user = credentials->getUser();
                                if (user)
                                {
                                    spdlog::info("Upgrading "
                                               + *user + " to a websocket");
                                    authenticated = true;
                               }
                            }
                        }
                        catch (const std::exception &e)
                        {
                            spdlog::warn("Authorization failed because: "
                                       + std::string {e.what()});
                        }
                    }
                }
            }
            if (authenticated)
            {
                spdlog::info(
                    "HTTPSesssion::onRead webSocket upgrade requested");
                // Disable the timeout.
                // The websocket::stream uses its own timeout settings.
                boost::beast::get_lowest_layer(derived().stream())
                                              .expires_never();
                // Create a websocket session, transferring ownership
                // of both the socket and the HTTP request.
                return ::makeWebSocketSession(
                    sessionIdentifier,
                    derived().releaseStream(),
                    mCallbackHandler,
                    mParser->release());
            }
            else
            {
                queueWrite(::createForbiddenResponse(
                   "Websocket upgrade denied because of invalid credentials",
                    mParser->release()));
            }
        }
        else
        {
            // Send the response
            if (authenticated)
            {
                queueWrite(::handleRequest(jsonWebToken,
                                           *mDocumentRoot,
                                           mParser->release(),
                                           mCallbackHandler));
            }
            else
            {
                queueWrite(::createForbiddenResponse("Invalid credentials",
                                                     mParser->release()));
            }
        }
        // If we aren't at the queue limit, try to pipeline another request
        if (mResponseQueue.size() < mQueueLimit){doRead();}
    }

    void queueWrite(boost::beast::http::message_generator response)
    {
        // Allocate and store the work
        mResponseQueue.push(std::move(response));

        // If there was no previous work, start the write loop
        if (mResponseQueue.size() == 1){doWrite();}
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    void doWrite()
    {
        if (!mResponseQueue.empty())
        {
            bool keepAlive = mResponseQueue.front().keep_alive();

            boost::beast::async_write(
                derived().stream(),
                std::move(mResponseQueue.front()),
                boost::beast::bind_front_handler(
                    &HTTPSession::onWrite,
                    derived().shared_from_this(),
                    keepAlive));
        }
    }

    void onWrite(const bool keepAlive,
                 boost::beast::error_code errorCode,
                 const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical("HTTPSession::onWrite write failed with "
                               + std::string {errorCode.what()});
            }
            return;
        }

        if (!keepAlive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return derived().closeConnection();
        }

        // Resume the read if it has been paused
        if (mResponseQueue.size() == mQueueLimit){doRead();}

        mResponseQueue.pop();

        doWrite();
    }
protected:
    boost::beast::flat_buffer mBuffer;
private:
    // Access the derived class, this is part of
    // the Curiously Recurring Template Pattern idiom.
    Derived& derived()
    {
        return static_cast<Derived &> (*this);
    }
    std::shared_ptr<const std::string> mDocumentRoot;
    std::shared_ptr<MLReview::Service::Handler> mCallbackHandler{nullptr};
    std::shared_ptr<UAuthenticator::IAuthenticator> mAuthenticator{nullptr};
    std::queue<boost::beast::http::message_generator> mResponseQueue;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> mParser;
    std::chrono::seconds mTimeOut{30};
    size_t mQueueLimit{16};
    size_t mMaximumMessageSizeInBytes{10000};
};

// Handles a plain HTTP connection
class PlainHTTPSession : public ::HTTPSession<::PlainHTTPSession>,
                         public std::enable_shared_from_this<::PlainHTTPSession>
{
public:     
    // Create the session
    PlainHTTPSession(
        boost::beast::tcp_stream &&stream,
        boost::beast::flat_buffer &&buffer,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
        std::shared_ptr<UAuthenticator::IAuthenticator> &authenticator) :
        ::HTTPSession<::PlainHTTPSession>(
            std::move(buffer),
            documentRoot,
            callbackHandler,
            authenticator),
        mStream(std::move(stream))
    {
        spdlog::debug("PlainHTTPSession: Launching HTTP session...");
    }
    // Called by the base class
    [[nodiscard]] boost::beast::tcp_stream& stream()
    {
        return mStream;
    }
    // Called by the base class
    [[nodiscard]] boost::beast::tcp_stream releaseStream()
    {
        return std::move(mStream);
    }
    // Start the session
    void run()
    {
        this->doRead();
    }
    // Called by the base class to end the connection
    void closeConnection()
    {
        // Send a TCP shutdown
        boost::beast::error_code errorCode;
        mStream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                                  errorCode);

        // At this point the connection is closed gracefully
    }
private:
    boost::beast::tcp_stream mStream;
};

// Handles an SSL HTTP connection
class SSLHTTPSession : public ::HTTPSession<::SSLHTTPSession>,
                       public std::enable_shared_from_this<::SSLHTTPSession>
{
public:
    // Create the http_session
    SSLHTTPSession(
        boost::beast::tcp_stream &&stream,
        boost::asio::ssl::context &context,
        boost::beast::flat_buffer &&buffer,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
        std::shared_ptr<UAuthenticator::IAuthenticator> &authenticator) :
        ::HTTPSession<::SSLHTTPSession>(
            std::move(buffer),
            documentRoot,
            callbackHandler,
            authenticator),
        mStream(std::move(stream), context)
    {
        spdlog::debug("SSLHTTPSession: Launching HTTP session...");
    }
    // Start the session
    void run()
    {
        // Set the timeout.
        boost::beast::get_lowest_layer(mStream).expires_after(mTimeOut);

        // Perform the SSL handshake
        // Note, this is the buffered version of the handshake.
        mStream.async_handshake(
            boost::asio::ssl::stream_base::server,
            mBuffer.data(),
            boost::beast::bind_front_handler(
                &::SSLHTTPSession::onHandshake,
                shared_from_this()));
    }
    // Called by the base class
    boost::asio::ssl::stream<boost::beast::tcp_stream> &stream()
    {
        return mStream;
    }

    // Called by the base class
    boost::asio::ssl::stream<boost::beast::tcp_stream> releaseStream()
    {
        return std::move(mStream);
    }
 
    // Called by the base class to end the connection
    void closeConnection()
    {   
        // Set the timeout.
        boost::beast::get_lowest_layer(mStream).expires_after(mTimeOut);

        // Perform the SSL shutdown
        mStream.async_shutdown(
            boost::beast::bind_front_handler(
                &::SSLHTTPSession::onShutdown,
                shared_from_this()));
    }   
private:
    void onHandshake(
        boost::beast::error_code errorCode,
        const size_t bytesUsed)
    {
        if (errorCode)
        {
            spdlog::error("SSLSession::onHandshake handshake error");
            return;
        }

        // Consume the portion of the buffer used by the handshake
        mBuffer.consume(bytesUsed);

        doRead();
    }
    void onShutdown(boost::beast::error_code errorCode)
    {
         if (errorCode)
        {
            spdlog::error("SSLSession::onShutdown shutdown error");
            return; 
        }
        // At this point the connection is closed gracefully
    }
    boost::asio::ssl::stream<boost::beast::tcp_stream> mStream;
    std::chrono::seconds mTimeOut{30};
};

///--------------------------------------------------------------------------///
///                                Detect Session                            ///
///--------------------------------------------------------------------------///
// Detects SSL handshakes
class DetectSession : public std::enable_shared_from_this<::DetectSession>
{
public:
    DetectSession(
        boost::asio::ip::tcp::socket &&socket,
        boost::asio::ssl::context& sslContext,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<MLReview::Service::Handler> &callbackHandler,
        std::shared_ptr<UAuthenticator::IAuthenticator> &authenticator) :
        mStream(std::move(socket)),
        mSSLContext(sslContext),
        mDocumentRoot(documentRoot),
        mCallbackHandler(callbackHandler),
        mAuthenticator(authenticator)
    {
    }

    // Launch the detector
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(
            mStream.get_executor(),
            boost::beast::bind_front_handler(
                &::DetectSession::onRun,
                this->shared_from_this()));
    }

    // Launch the detector
    void onRun()
    {
        // Set the timeout.
        boost::beast::get_lowest_layer(mStream)
           .expires_after(mStreamTimeOut);

        // Detect a TLS handshake
        boost::beast::async_detect_ssl(
            mStream,
            mBuffer,
            boost::beast::bind_front_handler(
                &::DetectSession::onDetect,
                shared_from_this()));
    }
private:
    void onDetect(boost::beast::error_code errorCode,
                  const bool result)
    {   
        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical(
                   "DetectSession::onDetect: Failed to detect; failed with: "
                  + std::string {errorCode.what()});
            }
            return;
        }

        if (result)
        {
            // Launch SSL session
            std::make_shared<::SSLHTTPSession>(
                std::move(mStream),
                mSSLContext,
                std::move(mBuffer),
                mDocumentRoot,
                mCallbackHandler,
                mAuthenticator)->run();
            return;
        }
        // Launch plain session
        std::make_shared<::PlainHTTPSession>(
            std::move(mStream),
            std::move(mBuffer),
            mDocumentRoot,
            mCallbackHandler,
            mAuthenticator)->run();

    }

    boost::beast::tcp_stream mStream;
    boost::asio::ssl::context& mSSLContext;
    std::shared_ptr<const std::string> mDocumentRoot;
    boost::beast::flat_buffer mBuffer;
    std::shared_ptr<MLReview::Service::Handler> mCallbackHandler{nullptr};
    std::shared_ptr<UAuthenticator::IAuthenticator> mAuthenticator{nullptr};
    std::chrono::seconds mStreamTimeOut{30};
};

}
#endif
