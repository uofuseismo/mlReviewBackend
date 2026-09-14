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
#include "mlReview/service/catalog/resource.hpp"
#include "mlReview/service/stations/resource.hpp"
#include "mlReview/service/waveforms/resource.hpp"
#include "mlReview/webServer/listener.hpp"

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

struct ProgramOptions
{
    boost::asio::ip::address address{boost::asio::ip::make_address("0.0.0.0")};
    std::filesystem::path documentRoot{"./"}; 
    int nThreads{1};
    unsigned short port{80};
    bool helpOnly{false};
};

/// @brief Parses the command line options.
[[nodiscard]] ::ProgramOptions parseCommandLineOptions(int argc, char *argv[])
{
    ::ProgramOptions result;
    boost::program_options::options_description desc(
R"""(
The mlReviewBackend is the API for the mlReview frontend.
Example usage:
    mlReviewBackend --address=127.0.0.1 --port=8080 --document_root=./ --n_threads=1
Allowed options)""");
    desc.add_options()
        ("help",    "Produces this help message")
        ("address", boost::program_options::value<std::string> ()->default_value("0.0.0.0"),
                    "The address at which to bind")
        ("port",    boost::program_options::value<uint16_t> ()->default_value(80),
                    "The port on which to bind")
        ("document_root", boost::program_options::value<std::string> ()->default_value("./"),
                    "The document root in case files are served")
        ("n_threads", boost::program_options::value<int> ()->default_value(1),
                     "The number of threads");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm); 
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        result.helpOnly = true;
        return result;
    }
    if (vm.count("address"))
    {
        auto address = vm["address"].as<std::string>();
        if (address.empty()){throw std::invalid_argument("Address is empty");}
        result.address = boost::asio::ip::make_address(address); 
    }
    if (vm.count("port"))
    {
        auto port = vm["port"].as<uint16_t> ();
        result.port = port;
    }
    if (vm.count("document_root"))
    {
        auto documentRoot = vm["document_root"].as<std::string>();
        if (documentRoot.empty()){documentRoot = "./";}
        if (!std::filesystem::exists(documentRoot))
        {
            throw std::runtime_error("Document root: " + documentRoot
                                   + " does not exist");
        }
        result.documentRoot = documentRoot;
    }
    if (vm.count("n_threads"))
    {
        auto nThreads = vm["n_threads"].as<int> ();
        if (nThreads < 1){throw std::invalid_argument("Number of threads must be positive");}
        result.nThreads = nThreads;
    }
    return result;
}

}

int main(int argc, char *argv[])
{ 
    ::ProgramOptions programOptions;
    try
    {
        programOptions = parseCommandLineOptions(argc, argv);
        if (programOptions.helpOnly){return EXIT_SUCCESS;}
    }
    catch (const std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    
    // Make an authenticator
//std::cout << std::getenv("LDAP_HOST") << std::endl;
    std::shared_ptr<UAuthenticator::IAuthenticator> authenticator
        = std::make_shared<UAuthenticator::LDAP> (
             std::getenv("LDAP_HOST"),
             std::stoi(std::getenv("LDAP_PORT")),
             std::getenv("LDAP_ORGANIZATION_UNIT"),
             std::getenv("LDAP_DOMAIN_COMPONENT"),
             UAuthenticator::LDAP::Version::Three,
             UAuthenticator::LDAP::TLSVerifyClient::Allow,
             "mlReview"); 
/*
    auto result = authenticator->authenticate("user", "password", UAuthenticator::Permissions::ReadWrite);           
    if (result != UAuthenticator::IAuthenticator::ReturnCode::Allowed)
    {
        spdlog::critical("Failed to validate user");
    }
    return 0;
*/

    auto aqmsDatabaseConnection = std::make_shared<MLReview::Database::Connection::PostgreSQL> ();
    aqmsDatabaseConnection->setUser(std::getenv("MLREVIEW_AQMS_DATABASE_READ_ONLY_USER"));
    aqmsDatabaseConnection->setPassword(std::getenv("MLREVIEW_AQMS_DATABASE_READ_ONLY_PASSWORD"));
    aqmsDatabaseConnection->setDatabaseName(std::getenv("MLREVIEW_AQMS_DATABASE_NAME"));
    aqmsDatabaseConnection->setAddress(std::getenv("MLREVIEW_AQMS_DATABASE_HOST"));
    aqmsDatabaseConnection->setPort(std::stoi(std::getenv("MLREVIEW_AQMS_DATABASE_PORT")));
    aqmsDatabaseConnection->setApplication("mlReviewClientBackend");

    auto mongoDatabaseConnection = std::make_shared<MLReview::Database::Connection::MongoDB> ();
    mongoDatabaseConnection->setUser(std::getenv("MLREVIEW_MONGODB_DATABASE_READ_WRITE_USER"));
    mongoDatabaseConnection->setPassword(std::getenv("MLREVIEW_MONGODB_DATABASE_READ_WRITE_PASSWORD"));
    mongoDatabaseConnection->setDatabaseName(std::getenv("MLREVIEW_MONGODB_DATABASE_NAME"));
    mongoDatabaseConnection->setAddress(std::getenv("MLREVIEW_MONGODB_DATABASE_HOST"));
    mongoDatabaseConnection->setPort(std::stoi(std::getenv("MLREVIEW_MONGODB_DATABASE_PORT")));
    mongoDatabaseConnection->setApplication("mlReviewClientBackend");
    mongoDatabaseConnection->connect();

    //getWaveform(*mlDatabaseConnection);

    auto acceptEventToAWS
        = std::make_unique<MLReview::Service::Actions::AcceptEventToAWS>
          (mongoDatabaseConnection);
    auto deleteEventFromAWS
        = std::make_unique<MLReview::Service::Actions::DeleteEventFromAWS>
          (mongoDatabaseConnection);
    auto catalogResource
        = std::make_unique<MLReview::Service::Catalog::Resource>
          (mongoDatabaseConnection);
    auto stationsResource
        = std::make_unique<MLReview::Service::Stations::Resource>
          (aqmsDatabaseConnection);
    auto waveformsResource
        = std::make_unique<MLReview::Service::Waveforms::Resource>
          (mongoDatabaseConnection);

    auto handler = std::make_shared<MLReview::Service::Handler> ();
    handler->insert(std::move(catalogResource));
    handler->insert(std::move(stationsResource));
    handler->insert(std::move(waveformsResource));
    handler->insert(std::move(acceptEventToAWS));
    handler->insert(std::move(deleteEventFromAWS));

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
