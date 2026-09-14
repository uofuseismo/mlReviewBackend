#ifndef MLREVIEW_SERVICE_STATIONS_STATION_HPP
#define MLREVIEW_SERVICE_STATIONS_STATION_HPP
#include <string>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
namespace MLReview::Service::Stations
{
/// @brief Defines a station location and name.
class Station
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Station();
    /// @brief Copy constructor.
    /// @param[in] station   The class from which to initialize this class.
    Station(const Station &station);
    /// @brief Move constructor.
    /// @param[in,out] station  The class from which to initialize this
    ///                         class.  On exit, station's behavior
    ///                         is undefined.
    Station(Station &&station) noexcept;
    /// @}

    /// @name Properties
    /// @{

    /// @brief Sets the network name.
    /// @param[in] network  The network name.
    void setNetwork(const std::string &network);
    /// @result The network name.
    /// @throws std::invalid_argument if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates that the network name was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] name  The station name.
    void setName(const std::string &name);
    /// @result The station name.
    /// @throws std::invalid_argument if \c haveStation() is false.
    [[nodiscard]] std::string getName() const;
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveName() const noexcept;

    /// @brief Sets the station latitude.
    /// @param[in] latitude  The station latitude in degrees.
    /// @throws std::invalid_argument if the latitude is not in
    ///         the range [-90,90].
    void setLatitude(double latitude);
    /// @result The station latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the station longitude.
    /// @param[in] longitude  The station longitude in degrees.
    void setLongitude(double longitude) noexcept;
    /// @result The station longitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief Sets the station elevation.
    /// @param[in] elevation  The station elevation in meters.
    void setElevation(double elevation);
    /// @result The station elevation in meters.
    /// @throws std::runtime_error if \c haveElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates that the elevation was set.
    [[nodiscard]] bool haveElevation() const noexcept;

    /// @brief Sets a string-based description of the station.
    /// @param[in] description  The station description - e.g., 
    ///                         Washington Dome, UT, USA.
    void setDescription(const std::string &description) noexcept;
    /// @result The station description.
    [[nodiscard]] std::string getDescription() const noexcept;

    /// @name On/Off Date
    /// @brief Sets the on and off date of the station.
    /// @param[in] onOffDate  onOffDate.first is the on date of the station
    ///                       and onOffDate.second is the off date.
    /// @throws std::invalid_argument if onOffDate.first >= onOffDate.second.
    void setOnOffDate(const std::pair<std::chrono::seconds, std::chrono::seconds> &onOffDate);
    /// @result The date when this station was turned on.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::seconds getOnDate() const;
    /// @result The date when this station may be turned off.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::seconds getOffDate() const;
    /// @result True indicates the on/off date was set.
    [[nodiscard]] bool haveOnOffDate() const noexcept;

    /// @result True indicates this station is operated/maintained by UUSS.
    [[nodiscard]] bool isLocal() const;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~Station();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] station  The station to copy to this.
    /// @result A deep copy of the station.
    Station& operator=(const Station &station);
    /// @brief Move assignment.
    /// @param[in,out] station   The station whose memory will be
    ///                          moved to this.  On exit, station's
    ///                          behavior is undefined.
    /// @result The station's memory moved to this.
    Station& operator=(Station &&station) noexcept;
    /// @}

private:
    class StationImpl;
    std::unique_ptr<StationImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Station &station);
}
#endif
