#include <string>
#include <algorithm>
#include <boost/format.hpp>
#include "mlReview/database/aqms/credit.hpp"

using namespace MLReview::Database::AQMS;

namespace
{

std::string convertString(const std::string &input)
{
    auto result = input;
    std::remove_if(result.begin(), result.end(), ::isspace);
    if (result.length() > 16)
    {
        result.resize(16);
    }
    return result;
}

std::string tableToString(const Credit::Table table)
{
    if (table == Credit::Table::Origin)
    {
        return "ORIGIN";
    }
    else if (table == Credit::Table::Netmag)
    {
        return "NETMAG";
    }
    else if (table == Credit::Table::Mec)
    {
        return "MEC";
    }
    throw std::runtime_error("Unhandled table");
}

}


class Credit::CreditImpl
{
public:
    std::string mReference;
    int64_t mIdentifier{-1};
    Credit::Table mTable;
    bool mHaveIdentifier{false};
    bool mHaveTable{false};
};

/// Constructor
Credit::Credit() :
    pImpl(std::make_unique<CreditImpl> ())
{
}

/// Copy constructor
Credit::Credit(const Credit &credit)
{
    *this = credit;
}

/// Move construtor
Credit::Credit(Credit &&credit) noexcept
{
    *this = std::move(credit);
}

/// Copy assignment
Credit& Credit::operator=(const Credit &credit)
{
    if (&credit == this){return *this;}
    pImpl = std::make_unique<CreditImpl> (*credit.pImpl);
    return *this;
}

/// Move assignment
Credit& Credit::operator=(Credit &&credit) noexcept
{
    if (&credit == this){return *this;}
    pImpl = std::move(credit.pImpl);
    return *this;
}

/// Identifier
void Credit::setIdentifier(const int64_t identifier)
{
    if (identifier < 0)
    {
        throw std::invalid_argument("Identifier must be non-negative");
    }
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

int64_t Credit::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not set");}
    return pImpl->mIdentifier;
}

bool Credit::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Table
void Credit::setTable(const Table table) noexcept
{
    pImpl->mTable = table;
    pImpl->mHaveTable = true;
}

Credit::Table Credit::getTable() const
{
    if (!haveTable()){throw std::runtime_error("Table not set");}
    return pImpl->mTable;
}

bool Credit::haveTable() const noexcept
{
    return pImpl->mHaveTable;
}

/// Reference
void Credit::setReference(const std::string &referenceIn)
{
    auto reference = ::convertString(referenceIn);
    if (reference.empty())
    {
        throw std::invalid_argument("Refererence is empty");
    }
    pImpl->mReference = reference;
}

std::string Credit::getReference() const
{
    if (!haveReference()){throw std::runtime_error("Reference not set");}
    return pImpl->mReference;
}

bool Credit::haveReference() const noexcept
{
    return !pImpl->mReference.empty();
}

/// Reset class
void Credit::clear() noexcept
{
    pImpl = std::make_unique<CreditImpl> ();
}

/// Destructor
Credit::~Credit() = default;

/// Insertion string
std::string MLReview::Database::AQMS::toInsertString(const Credit &credit)
{
    if (!credit.haveIdentifier())
    {
        throw std::invalid_argument("Identifier is not set");
    }
    if (!credit.haveTable())
    {
        throw std::invalid_argument("Table not set");
    }
    if (!credit.haveReference())
    {
        throw std::invalid_argument("Reference not set");
    }
    auto tableName = ::tableToString(credit.getTable());
    std::string insertStatement{"INSERT INTO credit "};
    std::string keys = {"(id, tname, refer)"};
    std::string values
        = str(boost::format(" VALUES (%1%, '%2%', '%3%')")
              %credit.getIdentifier()
              %tableName
              %credit.getReference()
             );
    // Put it all together
    insertStatement = insertStatement + keys + values + ";";
    return insertStatement;
}
