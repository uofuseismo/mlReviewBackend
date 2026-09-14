#ifndef MLREVIEW_SERVICE_CATALOG_ARRIVAL_HPP
#define MLREVIEW_SERVICE_CATALOG_ARRIVAL_HPP
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
namespace MLReview::Service::Catalog
{
class Arrival
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Arrival();
    /// @brief Copy constructor.
    /// @param[in] arrival  The arrival from which to initialize this class.
    Arrival(const Arrival &arrival);
    /// @brief Move constructor.
    /// @param[in,out] arrival  The arrival from which to initialize this class.
    ///                         On exit, arrival's behavior is undefined.
    Arrival(Arrival &&arrival) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the network code on which the pick was made.
    /// @param[in] network  The network code.
    /// @throws std::invalid_argument if network is empty.
    void setNetwork(const std::string &network);
    /// @result The network code.
    /// @throws std::runtime_error if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates that the network was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station  The station name.
    void setStation(const std::string &station);
    /// @result The station name.
    /// @throws std::invalid_argument if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const; 
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the channel.  In this case, this is a single-channel
    ///        vertical-only sensor.
    /// @param[in] verticalChannel  The vertical channel.
    void setChannels(const std::string &verticalChannel);
    /// @brief Sets the channel codes.  This is a three-component sensor.
    /// @param[in] verticalChannel  The vertical channel - e.g., ENZ.
    /// @param[in] northChannel     The north channel - e.g., ENN.
    /// @param[in] eastChannel      The east channel - e.g., ENE.
    void setChannels(const std::string &verticalChannel,
                     const std::string &northChannel,
                     const std::string &eastChannel);
    /// @result The vertical channel.
    /// @throws std::runtime_error if \c haveChannels() is false.
    [[nodiscard]] std::string getVerticalChannel() const;
    /// @result The north and east channels.
    /// @throws std::runtime_error if \c haveChannels() is false.
    [[nodiscard]] std::optional< std::pair<std::string, std::string> > getNonVerticalChannels() const;
    /// @result True indicates the channels were set.
    [[nodiscard]] bool haveChannels() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] location  The location code.
    /// @throws std::invalid_argument if location is empty.
    void setLocationCode(const std::string &location);
    /// @brief Sets the channel code on which the pick was made.
    /// @throws std::runtime_error if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates that the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;

    /// @brief Sets the arrival's seismic phase.
    /// @param[in] phase  The phase - e.g., P or S.
    void setPhase(const std::string &phase);
    /// @result The phase.
    /// @throws std::runtime_error if \c havePhase() is false.
    [[nodiscard]] std::string getPhase() const;
    /// @result True indicates the phase was set.
    [[nodiscard]] bool havePhase() const noexcept;

    /// @brief Sets the origin time.
    /// @param[in] time  The origin time (UTC) in microseconds since the epoch.
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @param[in] time   The origin time (UTC) in seconds since the epoch.
    void setTime(double time) noexcept;
    /// @result The origin time (UTC) in microseconds since the epoch.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates that the origin time was set.
    [[nodiscard]] bool haveTime() const noexcept;
    /// @}

    /// @name Optional Properties
    /// @{

    /// @brief Sets the travel time residual.
    /// @param[in] residual  The travel time residual (observed - estimated)
    ///                      in seconds.
    void setResidual(double residual) noexcept;
    /// @result The travel time residual (observed - estimated) in seconds.
    [[nodiscard]] std::optional<double> getResidual() const noexcept;

    /// @brief Sets the source-receiver distance in meters.
    void setDistance(double distance);
    /// @result The source-receiver distance in meters.
    [[nodiscard]] std::optional<double> getDistance() const noexcept;

    /// @brief Sets the source-to-receiver azimuth in degrees.
    /// @param[in] azimuth   The source-to-receiver azimuth in degrees measured
    ///                      positive east of north.
    void setAzimuth(double azimuth);
    /// @result The source-to-receiver azimuth in degrees measured positive
    ///         east of north.
    [[nodiscard]] std::optional<double> getAzimuth() const noexcept;
    /// @}

    /// @name Destructors
    /// @{
    
    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor
    ~Arrival();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] arrival  The arrival to copy to this.
    /// @result A deep copy of the arrival.
    Arrival& operator=(const Arrival &arrival);
    /// @brief Move assignment.
    /// @param[in,out] arrival  The arrival whose memory will be moved to this.
    ///                         On exit, arrival's behavior is undefined.
    /// @result The memory from arrival moved to this.
    Arrival& operator=(Arrival &&arrival) noexcept;
    /// @}
private:
    class ArrivalImpl;
    std::unique_ptr<ArrivalImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Arrival &arrival);
}
#endif
