#include <iostream>
#include <string>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "mlReview/database/connection/mongodb.hpp"

using namespace MLReview::Database::Connection;

class MongoDB::MongoDBImpl
{
public:
    MongoDBImpl()
    {
        mSessionPtr = &mSession;
    }
    mongocxx::client mSession;
    void *mSessionPtr{nullptr};
    std::string mConnectionString;
    std::string mUser;
    std::string mPassword;
    std::string mDatabaseName;
    std::string mAddress;
    std::string mApplication{"drp"};
    int mPort{27017};
};

/// Constructor
MongoDB::MongoDB() :
    pImpl(std::make_unique<MongoDBImpl> ())
{
}

/// Move constructor
MongoDB::MongoDB(MongoDB &&mongodb) noexcept
{
    *this = std::move(mongodb);
}

/// Move assignment
MongoDB& MongoDB::operator=(MongoDB &&mongodb) noexcept
{
    if (&mongodb == this){return *this;}
    pImpl = std::move(mongodb.pImpl);
    return *this;
}

/// Destructor
MongoDB::~MongoDB() = default; 

/// User
void MongoDB::setUser(const std::string &user)
{
    if (user.empty())
    {
        throw std::invalid_argument("user is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mUser = user;
}

std::string MongoDB::getUser() const
{
    if (!haveUser()){throw std::runtime_error("User not set");}
    return pImpl->mUser;
}

bool MongoDB::haveUser() const noexcept
{
    return !pImpl->mUser.empty();
}

/// Password
void MongoDB::setPassword(const std::string &password)
{
    if (password.empty())
    {
        throw std::invalid_argument("Password is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mPassword = password;
}

std::string MongoDB::getPassword() const
{
    if (!havePassword()){throw std::runtime_error("Password not set");}
    return pImpl->mPassword;
}

bool MongoDB::havePassword() const noexcept
{
    return !pImpl->mPassword.empty();
}

/// Address
void MongoDB::setAddress(const std::string &address)
{
    if (address.empty())
    {
        throw std::invalid_argument("Address is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mAddress = address;
}

std::string MongoDB::getAddress() const noexcept
{
    return pImpl->mAddress;
}

/// DB name
void MongoDB::setDatabaseName(const std::string &name)
{
    if (name.empty())
    {
        throw std::invalid_argument("Name is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mDatabaseName = name;
}

std::string MongoDB::getDatabaseName() const
{
    if (!haveDatabaseName()){throw std::runtime_error("Database name not set");}
    return pImpl->mDatabaseName;
}

bool MongoDB::haveDatabaseName() const noexcept
{
    return !pImpl->mDatabaseName.empty();
}

/// Port
void MongoDB::setPort(const int port)
{
    if (port < 0){throw std::invalid_argument("Port cannot be negative");}
    pImpl->mConnectionString.clear();
    pImpl->mPort = port;
}

int MongoDB::getPort() const noexcept
{
    return pImpl->mPort;
}

/// Application
void MongoDB::setApplication(const std::string &application)
{
    if (application.empty())
    {
        throw std::invalid_argument("Application is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mApplication = application;
}

std::string MongoDB::getApplication() const noexcept
{
    return pImpl->mApplication;
}

/// Drivername
std::string MongoDB::getDriver() noexcept
{
   return "mongodb";
}

/// Generate a connection string
std::string MongoDB::getConnectionString() const
{
    if (!pImpl->mConnectionString.empty()){return pImpl->mConnectionString;}
    if (!haveUser()){throw std::runtime_error("User not set");}
    if (!havePassword()){throw std::runtime_error("Password not set");}
    auto driver = getDriver();
    auto user = getUser();
    auto password = getPassword();
    auto address = getAddress();
    auto cPort = std::to_string(getPort()); 
    auto dbname = getDatabaseName();
    auto appName = getApplication();
    pImpl->mConnectionString = driver
                             + "://" + user
                             + ":" + password
                             + "@" + address
                             + ":" + cPort
                             + "/" + dbname
                             + "?connectTimeoutMS=10000"
                             + "&appName=" + appName;
    return pImpl->mConnectionString;
}

/// Connect
void MongoDB::connect()
{
    mongocxx::uri uri{getConnectionString()}; // Throws
    try
    {
        pImpl->mSession = std::move(mongocxx::client {uri});
        if (!pImpl->mSession)
        {
            throw std::runtime_error("Failed to connect to MongoDB");
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to connect to postgresql with error:\n"
                               + std::string{e.what()});
    }
}

bool MongoDB::isConnected() const noexcept
{
    return pImpl->mSession ? true : false;
}

/// Disconnect
void MongoDB::disconnect()
{
    //if (pImpl->mSession.is_connected()){pImpl->mSession.close();}
}

std::uintptr_t MongoDB::getSession() const
{
    return reinterpret_cast<std::uintptr_t> (pImpl->mSessionPtr);
}
