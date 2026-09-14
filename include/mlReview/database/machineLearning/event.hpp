#ifndef MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_EVENT_HPP
#define MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_EVENT_HPP
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
namespace MLReview::Database::MachineLearning::RealTime
{
class Origin;
class Magnitude;
}
namespace MLReview::Database::MachineLearning::RealTime
{
class Event
{
public:
    /// @brief The event's type.
    enum class Type
    {
        Unknown,           /*!< Unknown event type. */
        Earthquake,        /*!< Earthquake. */
        QuarryBlast        /*!< Quarry blast. */
    };
    /// @brief The monitoring region.
    enum class MonitoringRegion
    {
        Unknown,    /*!< Unknown monitoring region. */
        Utah,       /*!< Utah event. */
        Yellowstone /*!< Yellowstone event. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Event();
    /// @brief Copy constructor.
    /// @param[in] event  The event class from which to initialize this class.
    Event(const Event &event);
    /// @brief Move constructor.
    /// @param[in,out] event  The event class from which to initialize this 
    ///                       class.  On exit, event's behavior is undefined.
    Event(Event &&event) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the event identifier.
    /// @param[in] identifier  The event identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The event identifier.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates the event identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;
    /// @}


    /// @name Optional Properties
    /// @{

    /// @brief Sets the preferred origin.
    /// @param[in] origin  The preferred origin which must have a location
    ///                    and time.
    void setPreferredOrigin(const Origin &origin);
    /// @result The preferred origin information. 
    [[nodiscard]] std::optional<Origin> getPreferredOrigin() const noexcept;

    /// @brief Sets the event type.
    /// @param[in] type  The event type.
    void setType(Type type) noexcept;
    /// @result The event type.  By default this is unknown.
    [[nodiscard]] Type getType() const noexcept;

    /// @brief Sets the monitoring region.
    /// @param[in] region   The monitoring region.
    void setMonitoringRegion(MonitoringRegion region) noexcept;
    /// @result The monitoring region.  By default this is unknown.
    [[nodiscard]] MonitoringRegion getMonitoringRegion() const noexcept;

    /// @brief Sets the authority.
    /// @param[in] authority  The authority that generated the event.
    /// @throws std::invalid_argument if this is empty.
    void setAuthority(const std::string &authority);
    /// @result The authority that generated the event.
    [[nodiscard]] std::optional<std::string> getAuthority() const noexcept;

    /// @brief Sets the load date.
    /// @param[in] loadDate  The event's load date (UTC) in milliseconds since
    ///                      the epoch.
    void setLoadDate(const std::chrono::milliseconds &loadDate);
    /// @result The load date (UTC) in milliseconds since the epoch.
    [[nodiscard]] std::chrono::milliseconds getLoadDate() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Event();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] event  The event to copy to this.
    /// @result A deep copy of the event.
    Event& operator=(const Event &event);
    /// @brief Move assignment.
    /// @param[in,out] event  The event whose memory will be moved to this.
    ///                       On exit, event's behavior is undefined.
    /// @result The memory from event moved to this.
    Event& operator=(Event &&event) noexcept;
    /// @}
private:
    class EventImpl;
    std::unique_ptr<EventImpl> pImpl;
};
[[nodiscard]] nlohmann::json toObject(const Event &event);
}
#endif
