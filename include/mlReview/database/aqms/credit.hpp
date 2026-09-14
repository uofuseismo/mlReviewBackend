#ifndef MLREVIEW_DATABASE_AQMS_CREDIT_HPP
#define MLREVIEW_DATABASE_AQMS_CREDIT_HPP
#include <memory>
#include <string>
namespace MLReview::Database::AQMS
{
/// @class Credit "credit.hpp" "mlReview/database/aqms/credit.hpp"
/// @brief Defines whom to assign credit to in AQMS.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Credit
{
public:
    /// @brief The event's type.
    enum class Table
    {   
        Mec, /*!< The mechanism table. */
        Origin, /*!< Origin table. */
        Netmag, /*!< The network magnitude table. */
    };  
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Credit();
    /// @brief Copy constructor.
    /// @parma[in] credit  The credit from which to initialize this class.
    Credit(const Credit &credit);
    /// @brief Move constructor.
    /// @param[in,out] credit  The credit class from which this class will be
    ///                        initialized.  On exit, credit's behavior is
    ///                        undefined.
    Credit(Credit &&credit) noexcept; 
    /// @}

    /// @name Properties
    /// @{

    /// @brief The identifier in the table - e.g., the mecid, orid, magid.
    /// @param[in] identifier   The identifier.
    void setIdentifier(const int64_t identifier);
    /// @result The identifier.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates the identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

    /// @brief Sets the table.
    /// @param[in] table   The table.
    void setTable(Table table) noexcept;
    /// @result The table.
    [[nodiscard]] Table getTable() const;
    /// @result True indicates the table was set.
    [[nodiscard]] bool haveTable() const noexcept;

    /// @brief Sets the credit reference.
    /// @param[in] reference  The credit reference.
    void setReference(const std::string &reference);
    /// @result The credit reference.
    [[nodiscard]] std::string getReference() const;
    /// @result True indicates the reference was set.
    [[nodiscard]] bool haveReference() const noexcept;
    //// @}
    
    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Credit();
    /// @}

    /// @name Operators
    /// @{
 
    /// @brief Copy assignment.
    /// @param[in] credit   The credit to copy to this.
    /// @result A deep copy of the credit.
    Credit& operator=(const Credit &credit);
    /// @brief Move assignment.
    /// @param[in,out] credit  The credit whose memory will be moved to this.
    ///                        On exit, credit's behavior is undefined. 
    /// @result The memory from credit moved to this.
    Credit& operator=(Credit &&credit) noexcept;
    /// @}
private:
    class CreditImpl;
    std::unique_ptr<CreditImpl> pImpl;
};
[[nodiscard]] std::string toInsertString(const Credit &credit);
}
#endif
