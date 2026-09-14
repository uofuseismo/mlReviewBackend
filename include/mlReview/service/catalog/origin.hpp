#ifndef MLREVIEW_SERVICE_CATALOG_ORIGIN_HPP
#define MLREVIEW_SERVICE_CATALOG_ORIGIN_HPP
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
namespace MLReview::Service::Catalog
{
 class Arrival;
 class IMagnitude;
}
namespace MLReview::Service::Catalog
{
class Origin
{
public:
    enum class EventType
    {
        Unknown = 0,    /*!< Unknown event type. */
        Earthquake = 1, /*!< Earthquake. */
        QuarryBlast = 2 /*!< Quarry blast. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    /// @param[in] origin  The origin from which to initialize this class.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    /// @param[in,out] origin  The origin from which to initialize this class.
    ///                        On exit, origin's behavior is undefined.
    Origin(Origin &&origin) noexcept;
    /// @}

    /// @name Destructors
    /// @{
        
    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor
    ~Origin();
    /// @}

    /// @name Required Properties
    /// @{

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

    /// @brief Sets the origin's latitude.
    /// @param[in] latitude  The origin's latitude in degrees.
    /// @throws std::invalid_argument if the latitude is not in
    ///         the range [-90,90].
    void setLatitude(double latitude);
    /// @result The origin's latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that the latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the origin's longitude.
    /// @param[in] longitude  The origin's latitude in degrees.
    void setLongitude(double longitude) noexcept;
    /// @result The origin's longitude in degrees.
    /// @throws std::runtime_error if \c haveLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that the longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief Sets the origin's depth.
    /// @param[in] depth   The origin's depth in meters below sea-level.
    void setDepth(double depth);
    /// @result The origin's depth in meters w.r.t. to sea-level.
    /// @throws std::runtime_error if \c haveDepth() is false.
    [[nodiscard]] double getDepth() const;
    /// @result True indicates that the depth was set.
    [[nodiscard]] bool haveDepth() const noexcept;
    /// @}

    /// @name Optional Properties
    /// @{

    /// @brief Sets the preferred magnitude which must have a size and type.
    /// @param[in] magnitude  The preferred magnitude.
    void setPreferredMagnitude(const IMagnitude &magnitude);
    /// @result True indicates the preferred magnitude was set.
    [[nodiscard]] bool havePreferredMagnitude() const noexcept;

    void setArrivals(const std::vector<Arrival> &arrivals);
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const noexcept;

    /// @brief Sets the event type.
    void setEventType(EventType type) noexcept;
    /// @result The event type which, by default, is unknown.
    [[nodiscard]] EventType getEventType() const noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] origin  The origin to copy to this.
    /// @result A deep copy of the origin.
    Origin& operator=(const Origin &origin);
    /// @brief Move assignment.
    /// @param[in,out] origin  The origin whose memory will be moved to this.
    ///                        On exit, origin's behavior is undefined.
    /// @result The memory from origin moved to this.
    Origin& operator=(Origin &&origin) noexcept;
    /// @}
private:
    class OriginImpl;
    std::unique_ptr<OriginImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Origin &origin);
}
#endif
