#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
//#include <soci/soci.h>
#include <spdlog/spdlog.h>
#include <uAuthenticator/uAuthenticator.hpp>
#include "mlReview/database/connection/postgresql.hpp"
#include "mlReview/database/connection/mongodb.hpp"
#include "mlReview/service/handler.hpp"
#include "mlReview/service/actions/acceptEventToAWS.hpp"
#include "mlReview/service/actions/deleteEventFromAWS.hpp"
#include "mlReview/service/appSettings/getStadiaMapsAPIKey.hpp"
#include "mlReview/service/catalog/resource.hpp"
#include "mlReview/service/stations/resource.hpp"
#include "mlReview/service/waveforms/resource.hpp"
#include "mlReview/webServer/listener.hpp"
#include "mlReview/version.hpp"
#include "secretFile.hpp"

namespace
{

/*
void getWaveform(MLReview::Database::Connection::PostgreSQL &connection)
{
    auto session = reinterpret_cast<soci::session *> (connection.getSession());
    std::string data;
    *session << "SELECT waveform FROM real_time_schema.waveform WHERE identifier = 3821", soci::into(data);  
    std::cout << data << std::endl;
}
*/

#define APPLICATION_NAME "mlReviewBackend"

struct ProgramOptions
{
    std::string applicationName{APPLICATION_NAME};
    int verbosity{3};

    boost::asio::ip::address address{boost::asio::ip::make_address("127.0.0.1")};
    std::filesystem::path documentRoot{"./"}; 
    int nThreads{1};
    unsigned short port{8000};
    bool helpOnly{false};

    std::string ldapHost;
    uint16_t ldapPort{636};
    std::string ldapOrganizationalUnit;
    std::string ldapDomainComponent;

    std::string aqmsReadOnlyUser;
    std::string aqmsReadOnlyPassword;
    std::string aqmsDatabaseName;
    std::string aqmsHost;
    uint16_t aqmsPort{5432};

    std::string mongodbReadWriteUser;
    std::string mongodbReadWritePassword;
    std::string mongodbDatabaseName;
    std::string mongodbHost;
    uint16_t mongodbPort{27017};

    std::string mlReviewAPIURL;
    std::string mlReviewAPIKey;

    std::string stadiaMapsAPIKey;

    static ProgramOptions parseIniFile(const std::filesystem::path &iniFile)
    {
        if (!std::filesystem::exists(iniFile))
        {
            throw std::invalid_argument(std::string{iniFile}
                                      + " does not exist");
        }
        ProgramOptions options;
        // Parse the initialization file
        boost::property_tree::ptree propertyTree;
        boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

        options.applicationName
            = propertyTree.get<std::string> ("General.applicationName",
                                             options.applicationName);
        if (options.applicationName.empty())
        {
            options.applicationName = APPLICATION_NAME;
        }
        options.verbosity
            = propertyTree.get<int> ("General.verbosity", options.verbosity);

        auto stadiaKey
            = ::resolveSecret(
                 propertyTree,
                 "General.stadiaMapsAPIKey",
                 "General.stadiaMapsAPIKeyFile");
        if (stadiaKey){options.stadiaMapsAPIKey = *stadiaKey;}


        auto stringAddress
            = propertyTree.get<std::string> ("Beast.address", "127.0.0.1");
        if (stringAddress.empty())
        {
            throw std::invalid_argument("Beast address not set");
        }
        options.address = boost::asio::ip::make_address(stringAddress);

        options.port = propertyTree.get<uint16_t> ("Beast.port", options.port);
        if (options.port == 0)
        {
            throw std::invalid_argument("Port cannot be 0");
        }
       
        options.nThreads
            = propertyTree.get<int> ("Beast.numberOfThreads", options.nThreads);
        if (options.nThreads < 1)
        {
            throw std::invalid_argument("Number of threads must be positive");
        } 

        // A setting that must be given, either inline or in a file.  An
        // empty value is the same as an absent one: an empty password is
        // never what was meant, and it would otherwise surface as a
        // connection failure rather than a configuration error.
        auto requireSecret
            = [&propertyTree](const std::string &inlineKey,
                              const std::string &fileKey) -> std::string
        {
            auto value = ::resolveSecret(propertyTree, inlineKey, fileKey);
            if (!value || value->empty())
            {
                throw std::invalid_argument("Set " + inlineKey + " or "
                                          + fileKey);
            }
            return *value;
        };
        // A setting that must be given inline - these are not secrets.
        auto requireString
            = [&propertyTree](const std::string &key) -> std::string
        {
            auto value = propertyTree.get_optional<std::string> (key);
            if (!value || value->empty())
            {
                throw std::invalid_argument("Set " + key);
            }
            return *value;
        };
        // A port must be non-zero; 0 asks the OS to pick one, which for a
        // service we connect to is never right.
        auto getPort
            = [&propertyTree](const std::string &key,
                              const uint16_t defaultPort) -> uint16_t
        {
            auto port = propertyTree.get<uint16_t> (key, defaultPort);
            if (port == 0){throw std::invalid_argument(key + " cannot be 0");}
            return port;
        };

        options.aqmsReadOnlyUser
            = requireSecret("AQMS.readOnlyUser", "AQMS.readOnlyUserFile");
        options.aqmsReadOnlyPassword
            = requireSecret("AQMS.readOnlyPassword",
                            "AQMS.readOnlyPasswordFile");
        options.aqmsDatabaseName
            = requireSecret("AQMS.databaseName", "AQMS.databaseNameFile");
        options.aqmsHost
            = requireSecret("AQMS.host", "AQMS.hostFile");
        options.aqmsPort = getPort("AQMS.port", options.aqmsPort);

        options.mongodbReadWriteUser
            = requireSecret("MongoDB.readWriteUser",
                            "MongoDB.readWriteUserFile");
        options.mongodbReadWritePassword
            = requireSecret("MongoDB.readWritePassword",
                            "MongoDB.readWritePasswordFile");
        options.mongodbDatabaseName
            = requireSecret("MongoDB.databaseName",
                            "MongoDB.databaseNameFile");
        options.mongodbHost
            = requireSecret("MongoDB.host", "MongoDB.hostFile");
        options.mongodbPort = getPort("MongoDB.port", options.mongodbPort);

        options.mlReviewAPIURL
            = requireSecret("AWS.url", "AWS.urlFile");
        options.mlReviewAPIKey
            = requireSecret("AWS.key", "AWS.keyFile");

        options.ldapHost = requireSecret("LDAP.host", "LDAP.hostFile");
        options.ldapPort = getPort("LDAP.port", options.ldapPort);
        options.ldapOrganizationalUnit
            = requireString("LDAP.organizationalUnit");
        options.ldapDomainComponent
            = requireString("LDAP.domainComponent");

        return options;
    }
};

/// @brief Parses the command line options.
[[nodiscard]] 
std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mlReviewBackend is the API for the mlReview frontend.
Example usage:
    mlReviewBackend --ini=/path/to/config.ini
Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (), 
                 "The initialization file for this executable");
    boost::program_options::variables_map vm; 
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm); 
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {    
        std::cout << desc << std::endl;
        return {iniFile, true};
    }   
    if (vm.count("ini"))
    {    
        iniFile = vm["ini"].as<std::string>();
        if (!std::filesystem::exists(iniFile))
        {
            throw std::runtime_error("Initialization file: " + iniFile
                                   + " does not exist");
        }
    }    
    return {iniFile, false};
}

}

int main(int argc, char *argv[])
{ 
    spdlog::info("Launching mlReviewBackend version "
               + MLReview::Version::getVersionWithTag());

    std::filesystem::path iniFile;
    try
    {   
        auto [iniFileName, isHelp] = ::parseCommandLineOptions(argc, argv);
        if (isHelp){return EXIT_SUCCESS;}
        if (iniFileName.empty())
        {   
            throw std::runtime_error("No initialization file specified");
        }   
        iniFile = iniFileName;
    }
    catch (const std::exception &e)
    {
        spdlog::critical(e.what());
        return EXIT_FAILURE;
    }
    
    ::ProgramOptions programOptions;
    try
    {
        programOptions = ::ProgramOptions::parseIniFile(iniFile);
    }
    catch (const std::exception &e)
    {
        spdlog::critical(e.what());
        return EXIT_FAILURE;
    }

    // Make an authenticator
    std::shared_ptr<UAuthenticator::IAuthenticator> authenticator
        = std::make_shared<UAuthenticator::LDAP> (
             programOptions.ldapHost, //std::getenv("LDAP_HOST"),
             programOptions.ldapPort, //std::stoi(std::getenv("LDAP_PORT")),
             programOptions.ldapOrganizationalUnit, //std::getenv("LDAP_ORGANIZATION_UNIT"),
             programOptions.ldapDomainComponent, //std::getenv("LDAP_DOMAIN_COMPONENT"),
             UAuthenticator::LDAP::Version::Three,
             UAuthenticator::LDAP::TLSVerifyClient::Allow,
             programOptions.applicationName  //"mlReview"
             ); 

    auto aqmsDatabaseConnection = std::make_shared<MLReview::Database::Connection::PostgreSQL> ();
    aqmsDatabaseConnection->setUser(programOptions.aqmsReadOnlyUser); //std::getenv("MLREVIEW_AQMS_DATABASE_READ_ONLY_USER"));
    aqmsDatabaseConnection->setPassword(programOptions.aqmsReadOnlyPassword); //std::getenv("MLREVIEW_AQMS_DATABASE_READ_ONLY_PASSWORD"));
    aqmsDatabaseConnection->setDatabaseName(programOptions.aqmsDatabaseName); //std::getenv("MLREVIEW_AQMS_DATABASE_NAME"));
    aqmsDatabaseConnection->setAddress(programOptions.aqmsHost); //std::getenv("MLREVIEW_AQMS_DATABASE_HOST"));
    aqmsDatabaseConnection->setPort(programOptions.aqmsPort); //std::stoi(std::getenv("MLREVIEW_AQMS_DATABASE_PORT")));
    aqmsDatabaseConnection->setApplication(programOptions.applicationName); //"mlReviewClientBackend");

    auto mongoDatabaseConnection = std::make_shared<MLReview::Database::Connection::MongoDB> ();
    mongoDatabaseConnection->setUser(programOptions.mongodbReadWriteUser); //std::getenv("MLREVIEW_MONGODB_DATABASE_READ_WRITE_USER"));
    mongoDatabaseConnection->setPassword(programOptions.mongodbReadWritePassword); //std::getenv("MLREVIEW_MONGODB_DATABASE_READ_WRITE_PASSWORD"));
    mongoDatabaseConnection->setDatabaseName(programOptions.mongodbDatabaseName); //std::getenv("MLREVIEW_MONGODB_DATABASE_NAME"));
    mongoDatabaseConnection->setAddress(programOptions.mongodbHost); //std::getenv("MLREVIEW_MONGODB_DATABASE_HOST"));
    mongoDatabaseConnection->setPort(programOptions.mongodbPort); //std::stoi(std::getenv("MLREVIEW_MONGODB_DATABASE_PORT")));
    mongoDatabaseConnection->setApplication(programOptions.applicationName); //"mlReviewClientBackend");
    mongoDatabaseConnection->connect();

    //getWaveform(*mlDatabaseConnection);

    auto acceptEventToAWS
        = std::make_unique<MLReview::Service::Actions::AcceptEventToAWS>
          (mongoDatabaseConnection,
           programOptions.mlReviewAPIURL,
           programOptions.mlReviewAPIKey);
    auto deleteEventFromAWS
        = std::make_unique<MLReview::Service::Actions::DeleteEventFromAWS>
          (mongoDatabaseConnection,
           programOptions.mlReviewAPIURL,
           programOptions.mlReviewAPIKey);
    auto catalogResource
        = std::make_unique<MLReview::Service::Catalog::Resource>
          (mongoDatabaseConnection);
    auto stationsResource
        = std::make_unique<MLReview::Service::Stations::Resource>
          (aqmsDatabaseConnection);
    auto waveformsResource
        = std::make_unique<MLReview::Service::Waveforms::Resource>
          (mongoDatabaseConnection);
    auto stadiaKeyResource
        = std::make_unique<MLReview::Service::AppSettings::GetStadiaMapsAPIKey>
          (programOptions.stadiaMapsAPIKey);

    auto handler = std::make_shared<MLReview::Service::Handler> ();
    handler->insert(std::move(catalogResource));
    handler->insert(std::move(stationsResource));
    handler->insert(std::move(waveformsResource));
    handler->insert(std::move(acceptEventToAWS));
    handler->insert(std::move(deleteEventFromAWS));
    handler->insert(std::move(stadiaKeyResource));

    //const auto address = boost::asio::ip::make_address("127.0.0.1");
    //const auto port = static_cast<unsigned short> (8090);
    const auto documentRoot = std::make_shared<std::string> (programOptions.documentRoot);
    //const int nThreads{1};

    // The IO context is required for all I/O
    boost::asio::io_context ioContext{programOptions.nThreads};
    // The SSL context is required, and holds certificates
    boost::asio::ssl::context context{boost::asio::ssl::context::tlsv12};

    // Create and launch a listening port
    spdlog::info("Launching HTTP listeners on " 
               + programOptions.address.to_string()
               + ":" + std::to_string(programOptions.port));
    std::make_shared<MLReview::WebServer::Listener> (
        ioContext,
        context,
        boost::asio::ip::tcp::endpoint{programOptions.address,
                                       programOptions.port},
        documentRoot,
        handler,
        authenticator)->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> instances;
    instances.reserve(programOptions.nThreads - 1);
    for (int i = programOptions.nThreads - 1; i > 0; --i)
    {
        instances.emplace_back([&ioContext]
                               {
                                   ioContext.run();
                               });
    }
    ioContext.run();
    return EXIT_SUCCESS;
}
