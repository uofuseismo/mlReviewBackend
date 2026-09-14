#ifndef CURL_HPP
#define CURL_HPP
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <curl/curl.h>
namespace
{

/// Callback function that writes data to std::ostream
[[nodiscard]] size_t writeCURLData(void *buffer, const size_t size,
                                   const size_t nmemb, // Supposed to always be 1
                                   void *userParameters)
{
    if (userParameters)
    {
        std::ostream &os = *static_cast<std::ostream *>(userParameters);
        auto length = static_cast<std::streamsize> (size*nmemb);
        if (os.write(static_cast<char *> (buffer), length))
        {
            return length; // Return number of bytes taken care of
        }
    }
    return 0; // In this case I took care of zero bytes
}

[[nodiscard]]
nlohmann::json sendPutDeleteJSONRequest(const std::string &uri,
                                        const std::string &apiKey,
                                        const nlohmann::json &data,
                                        const std::string &method,
                                        const bool verbose)
{
    nlohmann::json jsonResponse;
    std::string errorMessage;
    CURL *curl{nullptr};
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        // Set the end point
        spdlog::debug("Endpoint is " + uri);
        auto returnCode = curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
        if (returnCode != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set URI");
        }
        // Verbosity
        if (verbose)
        {
            returnCode = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }
        else
        {
            returnCode = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        }
        if (returnCode != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set verbosity");
        }
        // In the header we pack the API key and let the API know we're
        // sending JSON
        struct curl_slist *headerList{nullptr};
        if (!apiKey.empty())
        {
            auto apiKeyHeader = "x-api-key:" + apiKey;
            headerList = curl_slist_append(headerList, apiKeyHeader.c_str());
        }
        headerList = curl_slist_append(headerList,
                                       "Accept: application/json");
        headerList = curl_slist_append(headerList,
                                       "Content-Type: application/json");
        returnCode = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        if (returnCode != CURLE_OK)
        {
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set CURL request header");
        }        
        // Setup the PUT request
        if (method == "PUT")
        {
            returnCode = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            if (returnCode != CURLE_OK)
            {
                curl_slist_free_all(headerList);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                throw std::runtime_error("Failed to specify PUT request");
            }
        }
        else if (method == "DELETE")
        {
            returnCode = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            if (returnCode != CURLE_OK)
            {   
                curl_slist_free_all(headerList);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                throw std::runtime_error("Failed to specify DELETE request");
            }   
        }
        else
        {
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Unhandled request: " + method);
        }
        auto dataForAPI = data.dump(-1);
        //spdlog::debug(dataForAPI);
        returnCode = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, 
                                      dataForAPI.c_str());
        if (returnCode != CURLE_OK)
        {
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set post fields");
        }
        // API will return JSON so get ready to read that
        returnCode = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                      &writeCURLData);
        if (returnCode != CURLE_OK)
        {
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set CURL data catcher");
        }
        std::ostringstream outputStream;
        returnCode = curl_easy_setopt(curl, CURLOPT_FILE, &outputStream);
        if (returnCode != CURLE_OK)
        {
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("Failed to set output stream handle");
        }
        // Finally send the message
        returnCode = curl_easy_perform(curl);
        if (returnCode != CURLE_OK)
        {
            errorMessage = std::string {curl_easy_strerror(returnCode)};
            spdlog::warn(errorMessage);
        }
        // Unpack the payload
        auto apiResponse = outputStream.str();
        try
        {
            jsonResponse = nlohmann::json::parse(apiResponse);
        }
        catch (const std::exception &e)
        {
            spdlog::info(apiResponse);
            spdlog::warn("Failed to parse result from API; failed with "
                       + std::string {e.what()});
            errorMessage = "Could not parse result from API";
        }
        // Clean up
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    else
    {
        errorMessage = "Error initializing curl";
    }
    curl_global_cleanup();
    if (!errorMessage.empty())
    {
        throw std::runtime_error("CURL request failed with: " + errorMessage);
    }
    return jsonResponse;
}

[[nodiscard]]
nlohmann::json sendPutJSONRequest(const std::string &uri,
                                  const std::string &apiKey,
                                  const nlohmann::json &data,
                                  const bool verbose = false)
{
    return sendPutDeleteJSONRequest(uri, apiKey, data, "PUT", verbose);
}

[[nodiscard]]                           
nlohmann::json sendDeleteJSONRequest(const std::string &uri,
                                     const std::string &apiKey,
                                     const nlohmann::json &data,
                                     const bool verbose = false)
{   
    return sendPutDeleteJSONRequest(uri, apiKey, data, "DELETE", verbose);
}   


}
#endif
