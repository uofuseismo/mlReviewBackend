#ifndef MLREVIEW_WAVE_SERVER_WAVEFORM_HPP
#define MLREVIEW_WAVE_SERVER_WAVEFORM_HPP
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
namespace MLReview::WaveServer
{
 class Segment;
}
namespace MLReview::WaveServer
{
/// @class Waveform "waveform.hpp" "mlReview/waveServer/waveform.hpp"
/// @brief A waveform is a collection of waveform segments.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Waveform
{
private:
    using VectorType = std::vector<Segment>;
public:
    using iterator = typename VectorType::iterator;
    using const_iterator = typename VectorType::const_iterator; 
public:
    /// @name Construtors
    /// @{

    /// @brief Constructor.
    Waveform();
    /// @brief Copy constructor.
    /// @param[in] waveform  The waveform from which to initialize this class.
    Waveform(const Waveform &waveform);
    /// @brief Move constructor.
    /// @param[in,out] waveform  The waveform from which to initialize
    ///                          this class.  On exit, waveform's behavior
    ///                          is undefined.
    Waveform(Waveform &&waveform) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the network code.
    /// @param[in] network  The network code - e.g., US.
    void setNetwork(const std::string &network);
    /// @result The network code.
    /// @throws std::runtime_error if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates the network code was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station  The station name - e.g., AHID.
    void setStation(const std::string &station);
    /// @result The station code.
    /// @throws std::runtime_error if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the channel code.
    /// @param[in] channel  The channel code - e.g., HHZ.
    void setChannel(const std::string &channel);
    /// @result The channel code.
    /// @throws std::runtime_error if \c haveChannel() is false.
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates the channel code was set.
    [[nodiscard]] bool haveChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] locationCode The location code - e.g., 00.
    void setLocationCode(const std::string &locationCode);
    /// @result The locaation code.
    /// @throws std::runtime_error if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;
    /// @}

    /// @name Data
    /// @{

    /// @brief Adds a waveform segment to the waveform.
    /// @param[in,out] segment   The segment to add to this waveform.
    ///                          On exit, segment's behavior is undefined.
    void addSegment(Segment &&segment);

    /// @result The number of segments.
    [[nodiscard]] int getNumberOfSegments() const noexcept;

    /// @brief Merge segments.  If the temporal difference between 
    ///        packet 1's end time + samplingPeriod and packet 1's start
    ///        time is less than samplingPeriodFactor*samplingPeriod
    ///        then the segments are merged.
    void mergeSegments(double samplingPeriodFactor = 0.5);
    /// @}


    /// @name Destructors
    /// @{

    /// @brief Releases memory and resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~Waveform();
    /// @}

    /// @name Operators and Iterators
    /// @{

    /// @result A deep copy of the input waveform.
    Waveform& operator=(const Waveform &waveform);
    /// @result The memory from waveform moved to this.
    Waveform& operator=(Waveform &&waveform) noexcept;

    /// @result A reference to the first waveform segment.
    iterator begin();
    /// @result A constant reference to the first waveform segment.
    const_iterator begin() const noexcept;
    /// @result A constant reference to the first waveform segment.
    const_iterator cbegin() const noexcept;

    /// @result A reference to the last waveform segment 
    iterator end();
    /// @result A reference to the last waveform segment.
    const_iterator end() const noexcept;
    /// @result A const reference to the waveform segment.
    const_iterator cend() const noexcept;

    /// @result The segment at the given index.
    const Segment& at(size_t i) const;
    /// @}
private:
    class WaveformImpl;
    std::unique_ptr<WaveformImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Waveform &waveform);
}
#endif
