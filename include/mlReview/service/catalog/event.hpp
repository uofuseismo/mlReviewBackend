#ifndef MLREVIEW_SERVICE_CATALOG_EVENT_HPP
#define MLREVIEW_SERVICE_CATALOG_EVENT_HPP
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
namespace MLReview::Service::Catalog
{
 class Origin;
}
namespace MLReview::Service::Catalog
{
class Event
{
public:
    /// @name Constructors
    /// @{

    /// @brief Construtor.
    Event();
    /// @brief Copy constructor.
    /// @param[in] event  The event from which to initialize this class.
    Event(const Event &event);
    /// @brief Move constructor.
    /// @param[in,out] event  The event from which to initialize this class.
    ///                       On exit, event's behavior is undefined.
    Event(Event &&event) noexcept;
    /// @brief Creates an event from a JSON object.
    explicit Event(const nlohmann::json &object);
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

    /// @brief Sets the preferred origin information.
    /// @param[in] origin   The preferred origin which must have the 
    ///                     hypocenter and origin time.
    void setPreferredOrigin(const Origin &origin);
    /// @result The preferred origin.
    /// @throws std::invalid_argument 
    [[nodiscard]] Origin getPreferredOrigin() const;
    /// @result True indicates the preferred origin was set.
    [[nodiscard]] bool havePreferredOrigin() const noexcept;
    /// @}

    /// @name Optional Properties
    /// @{

    /// @brief Sets the event identifier in the AQMS database.
    /// @param[in] identifier   The AQMS event identifiers.
    void setAQMSEventIdentifiers(const std::vector<int64_t> &identifiers) noexcept;
    /// @result The AQMS event identifier.
    [[nodiscard]] std::optional<std::vector<int64_t>> getAQMSEventIdentifiers() const noexcept;

    /// @brief Toggles the review status.
    /// @param[in] reviewed  If true then the event has been reviewed.
    void toggleReviewed(bool reviewed) noexcept;
    /// @result True indicates the event has been reviewed.
    [[nodiscard]] std::optional<bool> wasReviewed() const noexcept;

    /// @brief Toggles the status of being submitted to the cloud.
    /// @param[in] submitted  If true then the event was submitted to the cloud.
    void toggleSubmittedToCloudCatalog(bool submitted) noexcept;
    /// @result True indicates the event was submitted to the cloud catalog.
    [[nodiscard]] std::optional<bool> wasSubmittedToCloudCatalog() const noexcept;
    /// @}


    /// @name Destructors
    /// @{
    
    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor
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
