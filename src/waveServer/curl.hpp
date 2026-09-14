#ifndef CURL_IMPL_HPP
#define CURL_IMPL_HPP
#include <string>
#include <sstream>
#include <curl/curl.h>
namespace
{

/// Callback function that writes data to std::ostream
[[nodiscard]] size_t writeCURLData(void *buf, const size_t size,
                                   const size_t nmemb, void *userp)
{
    if (userp)
    {
        std::ostream &os = *static_cast<std::ostream *>(userp);
        auto len = static_cast<std::streamsize> (size*nmemb);
        if (os.write(static_cast<char *> (buf), len)){return len;}
    }
    return 0;
}

class CURLImpl
{
public:
    CURLImpl()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        mCurl = curl_easy_init();
        if (!mCurl){throw std::runtime_error("Failed to initialize curl");}
        auto code = curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, 1L);
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to disable progress");
        }
        code = curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, &writeCURLData);
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to initialize write function");
        } 
        code = curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1L);
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to enable follow redirects");
        }
        code = curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, mTimeOut);
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to set timeout");
        }
        code = curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        if (code != CURLE_OK)
        {   
            clear();
            throw std::runtime_error("Failed to set timeout");
        }
    }
    /// Gets data from a URL and puts the result in a std::string as a buffer.
    [[nodiscard]] std::string get(const std::string &url)
    {
        if (url.empty())
        {
            throw std::invalid_argument("URL is empty");
        }
        std::ostringstream outputStream;
        auto code = curl_easy_setopt(mCurl, CURLOPT_FILE, &outputStream);
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to set output stream handle");
        }
        code = curl_easy_setopt(mCurl, CURLOPT_URL, url.c_str());
        if (code != CURLE_OK)
        {
            clear();
            throw std::runtime_error("Failed to set URL");
        }
        code = curl_easy_perform(mCurl);
        if (code != CURLE_OK)
        {
            clear();
            auto errorMessage = std::string {curl_easy_strerror(code)};
            throw std::runtime_error("Failed to get data from: " + url 
                                   + " CURL failed with: " + errorMessage);
        }
        return outputStream.str();
    }
    /// Destructor
    ~CURLImpl()
    {
        clear();
        curl_global_cleanup();
    }
    /// Clears the curl handle
    void clear() noexcept
    {
        if (mCurl){curl_easy_cleanup(mCurl);}
        mCurl = nullptr;
    }
private:
    CURLImpl(const CURLImpl &curl) = delete;
    CURLImpl(CURLImpl &&curl) noexcept = delete;
    CURLImpl& operator=(const CURLImpl &curl) = delete;
    CURLImpl& operator=(CURLImpl &&curl) noexcept = delete;

    /// CURL handle
    CURL *mCurl{nullptr};
    /// CURL times out after mTimeOut seconds
    long mTimeOut{120L};
};

}
#endif
