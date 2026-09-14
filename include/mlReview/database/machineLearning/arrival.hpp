#ifndef MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_ARRIVAL_HPP
#define MLREVIEW_DATABASE_MACHINE_LEARNING_REAL_TIME_ARRIVAL_HPP
#include <memory>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
namespace MLReview::Database::MachineLearning::RealTime
{
class Arrival
{
public:
    enum class Phase
    {
        P,
        S
    };
    enum class ReviewStatus
    {   
        Automatic,  /*!< This is an automatic arrival. */
        Human       /*!< This is a human-reviewed arrival. */
    };  
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Arrival();
    /// @brief Copy constructor.
    /// @param[in] arrival  The arrival class from which to initialize
    ///                     this class.
    Arrival(const Arrival &arrival);
    /// @brief Move constructor.
    /// @param[in,out] arrival  The arrival class from which to initialize this
    ///                         class. On exit, arrival's behavior is undefined.
    Arrival(Arrival &&arrival) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets a unique arrival identification number.
    /// @param[in] identifier   The unique arrival identification number.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The unique arrival identification number.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates that the arrival identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

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
    void setPhase(const Phase phase) noexcept;
    /// @result The phase.
    /// @throws std::runtime_error if \c havePhase() is false.
    [[nodiscard]] Phase getPhase() const; 
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

    /// @name Optional Parameters
    /// @{

    /// @brief Sets the residual.
    /// @param[in] residual  The residual in seconds.
    void setResidual(double residual) noexcept;
    /// @result The observed - predicted time in seconds.
    [[nodiscard]] std::optional<double> getResidual() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
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
