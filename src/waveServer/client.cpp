#include <spdlog/spdlog.h>
#include "mlReview/waveServer/client.hpp"
#include "mlReview/waveServer/waveform.hpp"
#include "mlReview/waveServer/request.hpp"

using namespace MLReview::WaveServer;

/// Destructor
IClient::~IClient() = default;

/// Request data
std::vector<Waveform>
IClient::getData(const std::vector<Request> &requests) const
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
                result.push_back(Waveform {});
            }
        }
    }
    return result;
}
