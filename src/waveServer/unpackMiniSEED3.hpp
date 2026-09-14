#ifndef UNPACK_MINI_SEED_3_HPP
#define UNPACK_MINI_SEED_3_HPP
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <libmseed.h>
#include <spdlog/spdlog.h>
#include "mlReview/waveServer/waveform.hpp"
#include "mlReview/waveServer/segment.hpp"
namespace
{
[[nodiscard]] 
MLReview::WaveServer::Waveform unpack(char *data, size_t dataLength,
                                      const int8_t verbose = 0)
{
    MLReview::WaveServer::Waveform result;
    auto bufferLength = static_cast<uint64_t> (dataLength);
    uint64_t offset{0};
    bool isFirst{true};
    while (bufferLength - offset > MINRECLEN)
    {
        MS3Record *msr{nullptr};
        auto returnCode
            = msr3_parse(data + offset,
                         static_cast<uint64_t> (dataLength) - offset,
                         &msr, MSF_UNPACKDATA, verbose);
        if (returnCode == MS_NOERROR && msr)
        {
            // Waveform name
            std::array<char, 64> networkWork, stationWork, channelWork, locationCodeWork;
            std::fill(networkWork.begin(), networkWork.end(), '\0');
            std::fill(stationWork.begin(), stationWork.end(), '\0');
            std::fill(channelWork.begin(), channelWork.end(), '\0');
            std::fill(locationCodeWork.begin(), locationCodeWork.end(), '\0');
//#ifdef USE_MS_VERSION_315
            returnCode
                = ms_sid2nslc_n(msr->sid,
                                networkWork.data(), networkWork.size(),
                                stationWork.data(), stationWork.size(),
                                locationCodeWork.data(), locationCodeWork.size(),
                                channelWork.data(), channelWork.size());
/*
#else
            returnCode
                = ms_sid2nslc(msr->sid,
                              networkWork.data(), stationWork.data(),
                              locationCodeWork.data(), channelWork.data());
#endif
*/
            if (returnCode != MS_NOERROR)
            {
                if (msr){msr3_free(&msr);}
                throw std::runtime_error("Could not unpack sid");
            }
            // Copy waveform identification information
            if (isFirst)
            {
                try
                {
                    result.setNetwork(std::string {networkWork.data()});
                    result.setStation(std::string {stationWork.data()});
                    result.setChannel(std::string {channelWork.data()});
                    result.setLocationCode(
                        std::string {locationCodeWork.data()});
                    isFirst = false;
                }
                catch (const std::exception &e)
                {
                    if (msr)
                    {
                        msr3_free(&msr);
                        msr = nullptr;
                    }
                    throw std::runtime_error(
                        "Couldn't set waveform identifier information; failed with: "
                      + std::string {e.what()});
                }
            }
            auto startTime = static_cast<double> (msr->starttime)/NSTMODULUS;
            double samplingRate{msr->samprate};
            // Data
            auto nSamples = static_cast<int> (msr->numsamples);
            // Finally get the data
            MLReview::WaveServer::Segment::DataType dataType;
            if (msr->sampletype == 'i')
            {
                dataType = MLReview::WaveServer::Segment::DataType::Integer32;
            }
            else if (msr->sampletype == 'f')
            {
                dataType = MLReview::WaveServer::Segment::DataType::Float;
            }
            else if (msr->sampletype == 'd')
            {
                dataType = MLReview::WaveServer::Segment::DataType::Double;
            }
            else
            {
                spdlog::warn("Unhandled data format: "
                           + std::string {msr->sampletype} + "; skipping...");
                if (msr)
                {
                    msr3_free(&msr);
                    msr = nullptr;
                }
                offset = offset + msr->reclen;
                continue;
            }
            try
            {
                MLReview::WaveServer::Segment segment;
                segment.setStartTime(startTime);
                segment.setSamplingRate(samplingRate); 
                segment.setData(msr->datasamples, nSamples, dataType);
                result.addSegment(std::move(segment));
            }
            catch (const std::exception &e)
            {
                spdlog::warn("Failed to create segment.  Failed with "
                           + std::string {e.what()});
            }
            offset = offset + msr->reclen;
        } // End check on have record and no error
        // Release memory
        if (msr)
        {
            msr3_free(&msr);
            msr = nullptr;
        }
        // We're done
        if (returnCode != MS_NOERROR){break;}
    }
    return result;
}

MLReview::WaveServer::Waveform unpack(const std::string &data,
                                      const int8_t verbose = 0)
{
    auto copy = data;
    auto bufferLength = static_cast<uint64_t> (data.size());    
    return ::unpack(copy.data(), bufferLength);
}

}
#endif
