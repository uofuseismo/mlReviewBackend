#include <iostream>
#include <cmath>
#include <chrono>
#include <vector>
#include <nlohmann/json.hpp>
#include "mlReview/waveServer/segment.hpp"

using namespace MLReview::WaveServer;

class Segment::SegmentImpl
{
public:
    [[nodiscard]] int getNumberOfSamples() const noexcept
    {
        if (mDataType == Segment::DataType::Double)
        {
            return static_cast<int> (mData64f.size());
        }
        else if (mDataType == Segment::DataType::Float)
        {
            return static_cast<int> (mData32f.size());
        }
        else if (mDataType == Segment::DataType::Integer64)
        {
            return static_cast<int> (mData64i.size());
        }
        else if (mDataType == Segment::DataType::Integer32)
        {
            return static_cast<int> (mData32i.size());
        }
        return 0;
    }
    void updateEndTime()
    {
        mEndTime = mStartTime;
        if (mSamplingRate > 0 && mDataType != Segment::DataType::Undefined)
        {
            auto nSamples = getNumberOfSamples();
            if (nSamples > 0)
            {
                auto samplingPeriod = 1./mSamplingRate;
                auto endTime = mStartTime.count()*1.e-6
                             + (nSamples - 1)*samplingPeriod;
                auto iEndTimeMuS
                    = static_cast<int64_t> (std::round(endTime*1.e6));
                mEndTime = std::chrono::microseconds {iEndTimeMuS};
            }
        }
    }
    void clearVectors()
    {
        mData64f.clear();
        mData32f.clear();
        mData64i.clear();
        mData32i.clear();
    }
    std::vector<double> mData64f;
    std::vector<float> mData32f;
    std::vector<int64_t> mData64i;
    std::vector<int> mData32i;
    std::chrono::microseconds mStartTime{0};
    std::chrono::microseconds mEndTime{0};
    double mSamplingRate{0};
    Segment::DataType mDataType{Segment::DataType::Undefined};
};

/// Constructor
Segment::Segment() :
    pImpl(std::make_unique<SegmentImpl> ())
{
}

/// Copy constructor
Segment::Segment(const Segment &segment)
{
    *this = segment;
}

/// Move constructor
Segment::Segment(Segment &&segment) noexcept
{
    *this = std::move(segment);
}

/// Copy assignment
Segment& Segment::operator=(const Segment &segment)
{
    if (&segment == this){return *this;}
    pImpl = std::make_unique<SegmentImpl> (*segment.pImpl);
    return *this;
}

/// Move assignment
Segment& Segment::operator=(Segment &&segment) noexcept
{
    if (&segment == this){return *this;}
    pImpl = std::move(segment.pImpl);
    return *this;
}

/// Release memory/reset class
void Segment::clear() noexcept
{
    pImpl = std::make_unique<SegmentImpl> ();
}

/// Destructor
Segment::~Segment() = default;

/// Start time
void Segment::setStartTime(const double startTime) noexcept
{
    auto iStartTimeMuS = static_cast<int64_t> (std::round(startTime*1.e6));
    setStartTime(std::chrono::microseconds {iStartTimeMuS});
}

void Segment::setStartTime(
    const std::chrono::microseconds &startTime) noexcept
{
    pImpl->mStartTime = startTime;
    pImpl->updateEndTime();
}

std::chrono::microseconds Segment::getStartTime() const noexcept
{
    return pImpl->mStartTime;
}

std::chrono::microseconds Segment::getEndTime() const
{
    if (!haveSamplingRate())
    {
        throw std::runtime_error("Sampling rate not set");
    }
    return pImpl->mEndTime;
}

int Segment::getNumberOfSamples() const noexcept
{
    return pImpl->getNumberOfSamples();
}

/// Sampling rate
void Segment::setSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
    pImpl->updateEndTime();
}

double Segment::getSamplingRate() const
{
    if (!haveSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    return pImpl->mSamplingRate;
}

bool Segment::haveSamplingRate() const noexcept
{
    return (pImpl->mSamplingRate > 0);
}

/// Set data
void Segment::setData(const std::vector<int> &data)
{
    auto copy = data;
    setData(std::move(copy));
}

void Segment::setData(std::vector<int> &&data)
{
    pImpl->clearVectors();
    pImpl->mData32i = std::move(data);
    pImpl->mDataType = DataType::Integer32;
    pImpl->updateEndTime();
}

void Segment::setData(const std::vector<float> &data)
{
    auto copy = data;
    setData(std::move(copy));
}

void Segment::setData(std::vector<float> &&data)
{
    pImpl->clearVectors();
    pImpl->mData32f = std::move(data);
    pImpl->mDataType = DataType::Float;
    pImpl->updateEndTime();
}

void Segment::setData(const std::vector<int64_t> &data)
{
    auto copy = data;
    setData(std::move(copy));
}

void Segment::setData(std::vector<int64_t> &&data)
{
    pImpl->clearVectors();
    pImpl->mData64i = std::move(data);
    pImpl->mDataType = DataType::Integer64;
    pImpl->updateEndTime();
}

void Segment::setData(const std::vector<double> &data)
{
    auto copy = data;
    setData(std::move(copy));
}

void Segment::setData(std::vector<double> &&data)
{
    pImpl->clearVectors();
    pImpl->mData64f = std::move(data);
    pImpl->mDataType = DataType::Double;
    pImpl->updateEndTime();
}

void Segment::setData(const void *data, const int nSamples,
                      const DataType dataType)
{
    if (data == nullptr){throw std::invalid_argument("Data is null");}
    if (dataType == DataType::Double)
    {
        setData(reinterpret_cast<const double *> (data), nSamples);
    }
    else if (dataType == DataType::Float)
    {
        setData(reinterpret_cast<const float *> (data), nSamples);
    }
    else if (dataType == DataType::Integer64)
    {
	setData(reinterpret_cast<const int64_t *> (data), nSamples);
    }
    else if (dataType == DataType::Integer32)
    {
        setData(reinterpret_cast<const int32_t *> (data), nSamples);
    }
    else if (dataType == DataType::Undefined)
    {
        throw std::invalid_argument("Data type is undefined");
    }
}

void Segment::setData(const double *data, const int nSamples)
{
    pImpl->clearVectors();
    pImpl->mData64f.resize(nSamples);
    std::copy(data, data + nSamples, pImpl->mData64f.begin());    
    pImpl->mDataType = DataType::Double;
    pImpl->updateEndTime();
}

void Segment::setData(const float *data, const int nSamples)
{
    pImpl->clearVectors();
    pImpl->mData32f.resize(nSamples);
    std::copy(data, data + nSamples, pImpl->mData32f.begin());    
    pImpl->mDataType = DataType::Float;
    pImpl->updateEndTime();
}

void Segment::setData(const int64_t *data, const int nSamples)
{
    pImpl->clearVectors();
    pImpl->mData64i.resize(nSamples);
    std::copy(data, data + nSamples, pImpl->mData64i.begin());
    pImpl->mDataType = DataType::Integer64;
    pImpl->updateEndTime();
}

void Segment::setData(const int *data, const int nSamples)
{
    pImpl->clearVectors();
    pImpl->mData32i.resize(nSamples);
    std::copy(data, data + nSamples, pImpl->mData32i.begin());
    pImpl->mDataType = DataType::Integer32;
    pImpl->updateEndTime();
}

Segment::DataType Segment::getDataType() const noexcept
{
    return pImpl->mDataType;
}

/// Get data
template<typename U>
void Segment::getData(std::vector<U> *data) const
{
    if (data == nullptr){throw std::invalid_argument("data is null");}
    auto dataType = getDataType();
    if (dataType == Segment::DataType::Undefined)
    {
        throw std::runtime_error("No data set on segment");
    }
    data->resize(getNumberOfSamples());
    if (dataType == Segment::DataType::Integer32)
    {
        std::copy(pImpl->mData32i.begin(), pImpl->mData32i.end(),
                  data->begin());
    }
    else if (dataType == Segment::DataType::Float)
    {
        std::copy(pImpl->mData32f.begin(), pImpl->mData32f.end(),
                  data->begin());
    }
    else if (dataType == Segment::DataType::Integer64)
    {
        std::copy(pImpl->mData64i.begin(), pImpl->mData64i.end(),
                  data->begin());
    }
    else if (dataType == Segment::DataType::Double)
    {   
        std::copy(pImpl->mData64f.begin(), pImpl->mData64f.end(),
                  data->begin());
    }   
}

template<typename U>
std::vector<U> Segment::getData() const
{
    std::vector<U> data;
    getData(&data);
    return data;
}

/// 
nlohmann::json MLReview::WaveServer::toObject(const Segment &segment)
{
    if (!segment.haveSamplingRate())
    {
        throw std::invalid_argument("Sampling rate not set");
    }
    if (segment.getDataType() == Segment::DataType::Undefined)
    {
        throw std::runtime_error("No data set on segment");
    }
    nlohmann::json result;
    result["startTimeMuS"] = segment.getStartTime().count();
    result["samplingRateHZ"] = segment.getSamplingRate();
    if (segment.getDataType() == Segment::DataType::Integer32)
    {
        std::vector<int> data = segment.getData<int> ();
        result["dataType"] = "integer32";
        result["data"] = std::move(data);
    }
    else if (segment.getDataType() == Segment::DataType::Float)
    {
        std::vector<float> data = segment.getData<float> ();
        result["dataType"] = "float";
        result["data"] = std::move(data);
    }
    else if (segment.getDataType() == Segment::DataType::Integer64)
    { 
        std::vector<int64_t> data = segment.getData<int64_t> ();
        result["dataType"] = "integer64";
        result["data"] = std::move(data);
    }
    else if (segment.getDataType() == Segment::DataType::Double)
    {
        std::vector<double> data = segment.getData<double> ();
        result["dataType"] = "double";
        result["data"] = std::move(data);
    } 
    return result;
}

template void MLReview::WaveServer::Segment::getData(std::vector<double> *data) const;
template void MLReview::WaveServer::Segment::getData(std::vector<float> *data) const;
template void MLReview::WaveServer::Segment::getData(std::vector<int> *data) const;
template void MLReview::WaveServer::Segment::getData(std::vector<int64_t> *data) const;
