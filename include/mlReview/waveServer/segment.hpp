#ifndef MLREVIEW_WAVE_SERVER_SEGMENT_HPP
#define MLREVIEW_WAVE_SERVER_SEGMENT_HPP
#include <memory>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>
namespace MLReview::WaveServer
{
/// @class Segment "segment.hpp" "mlReview/waveServer/segment.hpp"
/// @brief Defines a waveform segment.  This is a continuous chunk of data.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Segment
{
public:
    /// @brief Defines the underlying data type.
    enum class DataType : int
    {
        Undefined = 0,
        Integer32 = 1,
        Float = 2,
        Integer64 = 3,
        Double = 4
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Segment();
    /// @brief Copy constructor.
    /// @param[in] segment  The waveform segment from which to initial
    ///                     this class.
    Segment(const Segment &segment);
    /// @brief Move constructor.
    /// @param[in,oiut] segment  The waveform segment from which to initialize
    ///                          this class.  On exit, segment's behavior is
    ///                          undefined.
    Segment(Segment &&segment) noexcept;
    /// @}

    /// @brief Sets the start time of the waveform segment.
    /// @param[in] startTime   The segment's start time (UTC) in seconds
    ///                        since the epoch. 
    void setStartTime(double starTime) noexcept;
    /// @brief Sets the start time of the waveform segment.
    /// @param[in] startTime   The segment's start time (UTC) in microseconds
    ///                        since the epoch. 
    void setStartTime(const std::chrono::microseconds &startTime) noexcept;
    /// @result The segment's start time (UTC) in microseconds since the epoch.
    ///         By default this is 0.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    /// @result The segment's end time (UTC) In microseconds since the epoch.
    /// @throws std::runtime_error if the sampling rate was not set.
    [[nodiscard]] std::chrono::microseconds getEndTime() const;

    /// @param[in] samplingRate  The sampling rate in Hz.
    /// @throw std::invalid_argument if the sampling rate is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The segment's sampling rate in Hz.
    /// @throws std::runtime_error if the sampling rate was not set.
    [[nodiscard]] double getSamplingRate() const;
    /// @result True indicates the sampling rate was set.
    [[nodiscard]] bool haveSamplingRate() const noexcept;

    void setData(const void *data, int nSamples, DataType dataType);
    void setData(const double *data, int nSamples);
    void setData(const float *data, int nSamples);
    void setData(const int64_t *data, int nSamples);
    void setData(const int *data, int nSamples);
    /// @brief Sets the segment's data.
    /// @param[in] data   The data to set.
    void setData(const std::vector<int> &data);
    void setData(const std::vector<float> &data);    
    void setData(const std::vector<double> &data);
    void setData(const std::vector<int64_t> &data);
    /// @brief Sets the segment's data.
    /// @param[in,out] data  The data to set.  On exit, data's behavior is
    ///                      undefined.
    void setData(std::vector<int> &&data);
    void setData(std::vector<float> &&data);    
    void setData(std::vector<double> &&data);
    void setData(std::vector<int64_t> &&data);
    /// @param[out] data  The data in the desired data type. 
    template<typename U> void getData(std::vector<U> *data) const;
    template<typename U> std::vector<U> getData() const;
    /// @result The data type.
    [[nodiscard]] DataType getDataType() const noexcept;
    /// @result The number of samples.
    [[nodiscard]] int getNumberOfSamples() const noexcept;

    [[nodiscard]] nlohmann::json toJSON() const noexcept; 

    /// @name Destructors
    /// @{

    /// @brief Releases memory and resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~Segment();
    /// @}

    /// @name Operators
    /// @{

    /// @result A deep copy of the waveform segment.
    Segment& operator=(const Segment &segment);
    Segment& operator=(Segment &&segment) noexcept;
    /// @}
private:
    class SegmentImpl;
    std::unique_ptr<SegmentImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Segment &segment);
}
#endif
