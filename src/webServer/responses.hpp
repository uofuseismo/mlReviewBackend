#ifndef HTTP_RESPONSES_HPP
#define HTTP_RESPONSES_HPP
#include <spdlog/spdlog.h>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
namespace
{

/// 200 everything is fine
template <class Body, class Allocator>
boost::beast::http::message_generator
createSuccessResponse(
    const std::string &message,
    boost::beast::http::request
    <   
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
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
    result.body() = message;
    result.prepare_payload();
    return result;
}


/// 400 bad request - the request is bad: e.g., malformed request syntax,
/// invalid request message framing, deceptive request routing, etc.
template <class Body, class Allocator>
boost::beast::http::message_generator
createBadRequestResponse(
    boost::beast::string_view why,
    boost::beast::http::request
    <   
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
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
}

/// 401 unauthorized - the user lacks authentication credentials for the
/// requested resource 
template <class Body, class Allocator>
boost::beast::http::message_generator
createUnauthorizedResponse(
    boost::beast::string_view why,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
    boost::beast::http::response<boost::beast::http::string_body> result
    {
        boost::beast::http::status::unauthorized,
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
}


/// 403 forbidden - this is like a 401 except (re)authenticating will make
/// no difference.
template <class Body, class Allocator>
boost::beast::http::message_generator
createForbiddenResponse(
    boost::beast::string_view why,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
    spdlog::info("Forbidden!");
    boost::beast::http::response<boost::beast::http::string_body> result
    {   
        boost::beast::http::status::forbidden,
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
}

/// 500 internal server error - the code threw and there's no better 5xx error.
/// This is a catch-all error.
/// 501 not implemented - the reuqest is okay, the server just didn't implement
/// it
template <class Body, class Allocator>
boost::beast::http::message_generator
createInternalServerErrorResponse(
    boost::beast::string_view why,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
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
    result.body() = "{\"status\":\"error\", \"reason\":\""
                  + std::string(why)
                  + "\"}";
    result.prepare_payload();
    return result;
}

/// 501 not implemented - the reuqest is okay, the server just didn't implement
/// it
template <class Body, class Allocator>
boost::beast::http::message_generator
createUnimplementedResponse(
    boost::beast::string_view why,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
{
    boost::beast::http::response<boost::beast::http::string_body> result
    {
        boost::beast::http::status::not_implemented,
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
}

/// CORS
template <class Body, class Allocator>
boost::beast::http::message_generator
createCORSResponse(
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request)
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
    //result.set(boost::beast::http::field::access_control_max_age,
    //           "3600");
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
}


}
#endif
