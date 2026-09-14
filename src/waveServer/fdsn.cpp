#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <chrono>
#include <spdlog/spdlog.h>
#include "mlReview/waveServer/fdsn.hpp"
#include "mlReview/waveServer/request.hpp"
#include "curl.hpp"
#include "unpackMiniSEED3.hpp"

#define TYPE "FDSN"
#define VERSION 1

using namespace MLReview::WaveServer;

namespace
{

std::string toDateTime(const std::chrono::microseconds &utcTimeMuS)
{
    auto utcTime = utcTimeMuS.count()*1.e-6;
    auto iUTCTime = static_cast<int64_t> (std::floor(utcTime));
    auto fraction = utcTime - static_cast<double> (iUTCTime);
    auto microSecond = static_cast<int> (std::lround(fraction*1.e6));
    // Create the epoch time
    std::chrono::seconds chronoSeconds{iUTCTime};
    std::chrono::system_clock::time_point timePoint{utcTimeMuS};
 
    auto dayPoint = std::chrono::floor<std::chrono::days> (timePoint);
    std::chrono::year_month_day yearMonthDay{dayPoint};

    auto year = static_cast<int> (yearMonthDay.year());
    auto month = static_cast<unsigned> (yearMonthDay.month());
    auto dayOfMonth = static_cast<unsigned> (yearMonthDay.day());

    const std::chrono::hh_mm_ss timeOfDay{timePoint - dayPoint};
    auto hour = static_cast<int> (timeOfDay.hours().count());
    auto minute = static_cast<int> (timeOfDay.minutes().count());
    auto second = static_cast<int> (timeOfDay.seconds().count());

    std::array<char, 64> buffer;
    std::fill(buffer.begin(), buffer.end(), '\0');
    if (microSecond != 0)
    {
        sprintf(buffer.data(), "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                year, month, dayOfMonth,
                hour, minute, second, microSecond);
    }
    else
    {
        sprintf(buffer.data(), "%04d-%02d-%02dT%02d:%02d:%02d",
                year, month, dayOfMonth,
                hour, minute, second);
    }
    return std::string {buffer.data()};
}

}

class FDSN::FDSNImpl
{
public:
    std::string mURL{"https://service.earthscope.org/fdsnws/dataselect/1/query"};
    //std::string mService{"fdsnws"};
    //int mVersion{VERSION};
};

/// Default constructor
FDSN::FDSN() :
    pImpl(std::make_unique<FDSNImpl> ())
{
}

/// Constructor with an end point
FDSN::FDSN(const std::string &url) :
    pImpl(std::make_unique<FDSNImpl> ())
{
    if (url.empty()){throw std::invalid_argument("URL is empty");} 
    pImpl->mURL = url;
//    if (pImpl->mURL.back() != '/'){pImpl->mURL = pImpl->mURL + "/";}
}

Waveform FDSN::getData(const Request &request) const
{
    Waveform result;
    if (!request.haveNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!request.haveStation())
    {
        throw std::invalid_argument("Station not set");
    }
    if (!request.haveChannel())
    {
        throw std::invalid_argument("Channel not set");
    }
    if (!request.haveStartAndEndTime())
    {
        throw std::invalid_argument("Start and end time not set");
    }
    std::string locationCode{"--"};
    if (request.haveLocationCode())
    {
        locationCode = request.getLocationCode();
    }
    auto query = pImpl->mURL //+ pImpl->mService
               //+ "/dataselect/" + std::to_string(pImpl->mVersion)
               //+ "/query
               + "?network=" + request.getNetwork()
               + "&station=" + request.getStation()
               + "&channel=" + request.getChannel()
               + "&location=" + locationCode
               + "&starttime=" + ::toDateTime(request.getStartTime())
               + "&endtime=" + ::toDateTime(request.getEndTime())
               + "&nodata=404";
    spdlog::info("Performing FDSN query: " + query);
    std::string payload;
    try
    {
        ::CURLImpl curl;
        payload = curl.get(query);
        auto waveform = ::unpack(payload);
        waveform.mergeSegments();
        result = std::move(waveform);
        if (result.getNumberOfSegments() > 0){spdlog::debug("success: " + query);}
//std::cout << payload << std::endl;
//std::cout << payload.size() << std::endl;
/*
std::ofstream outfile("yhh.mseed", std::ofstream::binary | std::ios::out);
size_t size = payload.size();
outfile.write(&payload[0], payload.size());
outfile.close();
payload.resize(2560);
std::ifstream infile("yhh.mseed", std::ofstream::binary | std::ios::in);
infile.read(payload.data(), payload.size());
auto waveform = ::unpack(payload);
auto obj = toObject(waveform);
std::cout<< obj << std::endl;
*/
    }
    catch (const std::exception &e)
    {
        auto error = "CURL request failed with: "
                   + std::string{e.what()};
        throw std::runtime_error(error);
    }
    return result;
}

std::string FDSN::getType() const noexcept
{
    return TYPE;
}

/// Destructor
FDSN::~FDSN() = default;
