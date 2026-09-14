#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <soci/soci.h>
#include "mlReview/service/stations/resource.hpp"
#include "mlReview/service/stations/response.hpp"
#include "mlReview/service/stations/station.hpp"
#include "mlReview/database/connection/postgresql.hpp"
#include "mlReview/messages/error.hpp"

#define RESOURCE_NAME "stations"

using namespace MLReview::Service::Stations;

namespace
{

std::chrono::seconds now()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>
           (now.time_since_epoch());
}

nlohmann::json toObject(const std::vector<Station> &stations,
                        const bool getLocal = false,
                        const bool getActive = false)
{
    auto now = ::now();
    nlohmann::json result;
    for (const auto &station : stations)
    {
        try
        {
            bool keep{true};
            if (getActive)
            {
                if (station.getOffDate() >= now) 
                {
                    keep = true;
                }
                else
                {
                    keep = false;
                    continue;
                }
            }
            if (getLocal)
            {
                if (station.isLocal())
                {
                    keep = true;
                }
                else
                {
                    keep = false;
                    continue;
                }
            }
            if (keep){result.push_back(toObject(station));}
        }
        catch (const std::exception &e)
        {
            spdlog::warn("Failed to pack station because "
                       + std::string {e.what()});
        }
    }
    return result;
}

/// Generates a catalog from the application database
std::vector<Station>
getStations(MLReview::Database::Connection::PostgreSQL &connection)
{
    std::vector<Station> stations;
    if (!connection.isConnected())
    {
        connection.connect();
        if (!connection.isConnected())
        {
            spdlog::critical("Could not connect to AQMS database");
            return stations;
        }
    }
    auto session
        = reinterpret_cast<soci::session *> (connection.getSession());
    std::string query{"SELECT net, sta, staname, lat, lon, elev, EXTRACT(epoch FROM ondate) AS ondate, EXTRACT(epoch FROM offdate) AS offdate FROM station_data"};
    soci::rowset<soci::row> stationRows = (session->prepare << query);
    for (soci::rowset<soci::row>::const_iterator it = stationRows.begin();
         it != stationRows.end(); ++it)
    {   
        Station station;
        const auto &row = *it;
        try
        {
            station.setNetwork(row.get<std::string> (0));
            station.setName(row.get<std::string> (1));
            station.setDescription(row.get<std::string> (2, ""));
            station.setLatitude(row.get<double> (3));
            station.setLongitude(row.get<double> (4));
            station.setElevation(row.get<double> (5));
            auto onDate
                = static_cast<int64_t> (std::floor(row.get<double> (6)));
            auto offDate
                = static_cast<int64_t> (std::floor(row.get<double> (7)));
            station.setOnOffDate(
                std::pair {std::chrono::seconds {onDate},
                           std::chrono::seconds {offDate}}); 
            stations.push_back(std::move(station));
        }
        catch (const std::exception &e)
        {
            spdlog::warn("Failed to create station because: "
                       + std::string{e.what()});
        }
    }
    return stations;
}

}

class Resource::ResourceImpl
{
public:
    ResourceImpl(
        std::shared_ptr<MLReview::Database::Connection::PostgreSQL> &aqmsConnection) :
        mAQMSConnection(aqmsConnection)
    {
        mStations = getStations(*mAQMSConnection);
    }
    ~ResourceImpl()
    {
        if (mQueryThread.joinable()){mQueryThread.join();}
    }
    std::thread mQueryThread;
    std::shared_ptr<MLReview::Database::Connection::PostgreSQL>
        mAQMSConnection{nullptr};
    std::vector<Station> mStations;
};

/// Constructor
Resource::Resource(
    std::shared_ptr<MLReview::Database::Connection::PostgreSQL> &aqmsConnection) :
    pImpl(std::make_unique<ResourceImpl> (aqmsConnection))
{
}

/// Destructor
Resource::~Resource() = default;

/// Resource name
std::string Resource::getName() const noexcept
{
    return RESOURCE_NAME;
}

/// Process request
std::unique_ptr<MLReview::Messages::IMessage> 
Resource::processRequest(const nlohmann::json &request)
{
    bool getActive{false};
    if (request.contains("getActive"))
    {
        getActive = request["getActive"].template get<bool> ();
    }
    bool getLocal{false};
    if (request.contains("getLocal"))
    {
        getLocal = request["getLocal"].template get<bool> ();
    }
/*
    // Figure out the query information
    auto now = std::chrono::duration_cast<std::chrono::seconds> (
        std::chrono::system_clock::now().time_since_epoch()); 
    auto endTime = now;
    auto startTime = now - std::chrono::seconds{14*86400};
    bool customQuery{false};
    if (request.contains("startTime"))
    {
        auto startTimeRequest
            = std::chrono::seconds
              {request["startTime"].template get<int64_t> ()};
        if (startTimeRequest != startTime)
        {
            startTime = startTimeRequest;
            customQuery = true;
        }
    }
    if (request.contains("endTime"))
    {   
        auto endTimeRequest
            = std::chrono::seconds
              {request["endTime"].template get<int64_t> ()};
        if (endTimeRequest != endTime)
        {
            endTime = endTimeRequest;
            customQuery = true;
        } 
    }
    // Do the times make sense?
    if (endTime <= startTime)
    {
        throw std::invalid_argument(
            "Catalog end time must be greater than start time");
    }
    std::string format{"json"};
    if (request.contains("format"))
    {
        auto requestFormat
           = request["format"].template get<std::string> ();
        if (requestFormat != format)
        {

        }
        else
        {
            if (requestFormat != "json")
            {
                throw std::invalid_argument("Unhandled catalog format: "
                                          + requestFormat);
            }
        }
    }
*/
    auto response = std::make_unique<Response> ();
    response->setMessage("Successful response to station list request");
    response->setData(::toObject(pImpl->mStations, getLocal, getActive));
    return response;
}
