#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>
#include "mlReview/waveServer/multiClient.hpp"
#include "mlReview/waveServer/waveform.hpp"
#include "mlReview/waveServer/segment.hpp"
#include "mlReview/waveServer/request.hpp"

#define TYPE "MultiClient"

using namespace MLReview::WaveServer;

namespace
{
double percentComplete(const Waveform &waveform,
                       const Request &request)
{
    if (waveform.getNumberOfSegments() < 1){return 0;}
    auto startTime = request.getStartTime();
    auto endTime = request.getEndTime();
    auto desiredDuration = endTime - startTime;
    if (desiredDuration == std::chrono::microseconds {0}){return 100;}
    // This is fairly easy
    std::chrono::microseconds missingDuration{0};
    if (waveform.getNumberOfSegments() == 1)
    {
        const auto &segment = waveform.at(0);
        if (segment.getStartTime() > startTime)
        {
            missingDuration = missingDuration
                            + (segment.getStartTime() - startTime);
        } 
        if (segment.getEndTime() < endTime)
        {
            missingDuration = missingDuration
                            + (endTime - segment.getEndTime());
        }
    }
    else
    {
        // This is a bit harder because we have gappy data.  Figure out
        // the start gap.
        auto nSegments = waveform.getNumberOfSegments();
        int startSegment =-1;
        for (int iSegment = 0; iSegment < nSegments; ++iSegment)
        {
            const auto &segment = waveform.at(iSegment);
            if (segment.getEndTime() >= startTime)
            {
                if (segment.getStartTime() >= startTime)
                {
                    missingDuration = missingDuration
                                    + (segment.getStartTime() - startTime);
                }
                startSegment = iSegment;
                break;
            }
        }
        // Never made it to the start - quit early
        if (startSegment < 0){return 0;}
        // Figure out the end gap.
        int endSegment = nSegments;
        for (int iSegment = nSegments - 1; iSegment >= startSegment; --iSegment)
        {
            const auto &segment = waveform.at(iSegment);
            if (segment.getStartTime() <= endTime)
            {
                if (segment.getEndTime() <= endTime)
                { 
                    missingDuration = missingDuration
                                    + (endTime - segment.getEndTime());
                }
                endSegment = iSegment;
                break;
            }
        }
        // Never made it to the end which should be impossible at this point.
        if (endSegment == nSegments)
        {
            return 0; 
        }
        // Tally gaps for every segment between (assuming segments are ordered
        // in time and there's no weird overlaps)
        for (int iSegment = startSegment; iSegment < endSegment; ++iSegment)
        {
             auto segment0 = waveform.at(iSegment);
             auto segment1 = waveform.at(iSegment + 1);            
             auto gap = segment1.getStartTime() - segment0.getEndTime();
             auto samplingPeriod = 1./segment0.getSamplingRate();
             if (gap.count()*1.e-6 > samplingPeriod + 0.5*samplingPeriod)
             {
                 missingDuration = missingDuration + gap;
             }
        }
    } 
    auto fraction = static_cast<double> (missingDuration.count())
                   /static_cast<double> (desiredDuration.count());
    return 100*(1 - fraction);
}
}

class MultiClient::MultiClientImpl
{
public:
    std::vector<std::pair<int, std::unique_ptr<IClient>>> mClients;
    double mCompleteTolerance{90};
};

/// Constructor
MultiClient::MultiClient() :
    pImpl(std::make_unique<MultiClientImpl> ())
{
}

/// 
void MultiClient::insert(std::unique_ptr<IClient> &&client, int priority)
{
    if (client == nullptr){throw std::invalid_argument("Clinet is null");}
    pImpl->mClients.push_back(std::pair {priority, std::move(client)});
    std::sort(pImpl->mClients.begin(), pImpl->mClients.end(),
              [](const auto &lhs, const auto &rhs)
              {
                 return lhs.first > rhs.first;
              }); 
}

/// Request data
std::vector<Waveform>
MultiClient::getData(const std::vector<Request> &requests) const
{
    std::vector<Waveform> result;
    // Create a unique set of requests
    std::vector<std::pair<Request, Waveform>> filledRequests;
    for (const auto &request: requests)
    {
        bool found{false};
        for (const auto &filledRequest : filledRequests)
        {
            if (request == filledRequest.first)
            {
                spdlog::debug("Duplicate request; saving");
                result.push_back(filledRequest.second);
                found = true;
                break;
            }
        }
        if (!found)
        {
            try
            {
                auto waveform = getData(request);
                result.push_back(waveform);
                filledRequests.push_back(
                    std::move(std::pair {request, std::move(waveform)}));
            }
            catch (const std::exception &e)
            {
                spdlog::warn("Failed to get data for");
                filledRequests.push_back(std::pair {request, Waveform {}});
                Waveform emptyWaveform;
                emptyWaveform.setNetwork(request.getNetwork());
                emptyWaveform.setStation(request.getStation());
                emptyWaveform.setChannel(request.getChannel());
                if (request.haveLocationCode())
                {
                    emptyWaveform.setLocationCode(request.getLocationCode());
                }
                result.push_back(emptyWaveform);
                filledRequests.push_back(
                    std::move(std::pair {request, std::move(emptyWaveform)}));
            }
        }
    }
    return result;
}


/// Get a waveform
Waveform MultiClient::getData(const Request &request) const
{
    Waveform result;
    double bestCompleteness{0};
    for (const auto &client : pImpl->mClients)
    {
        try
        {
            auto waveform = client.second->getData(request);
            if (!waveform.haveNetwork())
            {
                waveform.setNetwork(request.getNetwork());
            }
            if (!waveform.haveStation())
            {
                waveform.setStation(request.getStation());
            }
            if (!waveform.haveChannel())
            {
                waveform.setChannel(request.getChannel());
            }
            if (!waveform.haveLocationCode())
            {
                if (request.haveLocationCode())
                {
                    waveform.setLocationCode(request.getLocationCode());
                }
            }
            auto percentComplete = ::percentComplete(waveform, request); 
            // Good enough to keep
            if (percentComplete >= pImpl->mCompleteTolerance)
            {
                result = std::move(waveform);
                break;
            }
            else
            {
                // Save the best that we find
                if (percentComplete > bestCompleteness)
                {
                    result = std::move(waveform); 
                    bestCompleteness = percentComplete;
                }
            }
        }
        catch (const std::exception &e)
        {
            spdlog::warn("Failed to request data from client: "
                       + client.second->getType());
            //           + ".  Failed with "
            //           + std::string {e.what()}); 
        } 
    }
    return result;
}

/// Destructor
MultiClient::~MultiClient() = default;

/// Type
std::string MultiClient::getType() const noexcept
{
    return TYPE;
}
