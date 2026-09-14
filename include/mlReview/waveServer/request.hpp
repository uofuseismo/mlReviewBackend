#ifndef MLREVIEW_WAVE_SERVER_REQUEST_HPP
#define MLREVIEW_WAVE_SERVER_REQUEST_HPP
#include <memory>
#include <chrono>
#include <string>
#include <optional>
namespace MLReview::WaveServer
{
/// @name Request "request.hpp" "mlReview/waveServer/request.hpp"
/// @brief Defines a waveform request.  This involves a start time, end time,
///        station, network, channel, and location code.
class Request
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Request();
    /// @brief Copy constructor.
    /// @param[in] request   The request from which to initialize this class.
    Request(const Request &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    Request(Request &&request) noexcept;
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

    /// @brief Sets the start and end time.
    /// @param[in] startAndEndTime  The start and end time (UTC) in seconds
    ///                             since the epoch.
    /// @throws std::invalid_argument if
    ///         startAndEndTime.first >= startAndEndTime.second.
    void setStartAndEndTime(const std::pair<double, double> &startAndEndTime);
    /// @brief Sets the start and end time.
    /// @param[in] startAndEndTime  The start and end time (UTC) in microseconds
    ///                             since the epoch.
    void setStartAndEndTime(const std::pair<std::chrono::microseconds, std::chrono::microseconds> &startAndEndTime);
    /// @result The start time in microseconds (UTC) since the epoch.
    /// @throws std::runtime_error if \c haveStartAndEndTime() is false.
    [[nodiscard]] std::chrono::microseconds getStartTime() const;
    /// @result The end time in microseconds (UTC) since the epoch.
    /// @throws std::runtime_error if \c haveStartAndEndTime() is false.
    [[nodiscard]] std::chrono::microseconds getEndTime() const;
    /// @result True indicates the start and end time were set.
    [[nodiscard]] bool haveStartAndEndTime() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Releases all memory and resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~Request();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @result A deep copy of the request.
    Request& operator=(const Request &request);
    /// @brief Move assignment.
    /// @result The memory from request moved to this.
    Request& operator=(Request &&request) noexcept;
    /// @}
private:
    class RequestImpl;
    std::unique_ptr<RequestImpl> pImpl;
};
bool operator==(const Request &lhs, const Request &rhs);
bool operator!=(const Request &lhs, const Request &rhs);
}
#endif
