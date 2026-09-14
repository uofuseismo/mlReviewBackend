#ifndef MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_ORIGIN_HPP
#define MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_ORIGIN_HPP
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
namespace MLReview::Database::MachineLearning::RealTime
{
 class Arrival;
}
namespace MLReview::Database::MachineLearning::RealTime
{
class Origin
{
public:
    enum class ReviewStatus
    {
        Automatic,  /*!< This is an automatic origin. */
        Human,      /*!< This is a human-reviewed origin. */
        Finalized   /*!< This is a finalized origin. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    /// @param[in] origin  The origin class from which to initialize this class.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    /// @param[in,out] origin  The origin class from which to initialize this
    ///                        class.  On exit, origin's behavior is undefined.
    Origin(Origin &&origin) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the origin identifier.
    /// @param[in] identifier  The origin identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The origin identifier.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates that the origin identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

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

    /// @brief Sets the arrivals on the origin.
    void setArrivals(const std::vector<Arrival> &arrivals);
    /// @result The arrivals that built this origin.
    [[nodiscard]] std::vector<Arrival> getArrivals() const noexcept;
    /// @result A reference to the arrivals.
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const noexcept;

    /// @brief Sets the review status.
    /// @param[in] status  The review status.
    void setReviewStatus(ReviewStatus status) noexcept;
    /// @result The review status.
    [[nodiscard]] std::optional<ReviewStatus> getReviewStatus() const noexcept;

    /// @brief Sets the algorithm that created this origin.
    void setAlgorithm(const std::string &algorithm);
    /// @result The algorithm that created this origin.
    [[nodiscard]] std::optional<std::string> getAlgorithm() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Origin();
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
