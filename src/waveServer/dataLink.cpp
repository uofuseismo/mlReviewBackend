#include <iostream>
#include <string>
#include <array>
#include <set>
#include <chrono>
#include <libdali.h>
#include <libxml/parser.h>
#include "mlReview/waveServer/dataLink.hpp"
#include "mlReview/waveServer/request.hpp"
#include "unpackMiniSEED3.hpp"

#define TYPE "DataLink"

xmlNode *findNode(xmlNode *node, const char *nodeName)
{
    xmlNode *result{nullptr};
    if (node == nullptr){return result;}
    while (node)
    {
        if (node->type == XML_ELEMENT_NODE &&
            xmlStrcmp(node->name,
                      reinterpret_cast<const xmlChar *> (nodeName)) == 0)
        {
            return node; 
        }
        if (result == findNode(node->children, nodeName)){return result;}
        node = node->next; 
    }
    return nullptr;
}

std::set<std::string> parser(const char *content, size_t length)
{
    std::set<std::string> result;
    xmlDocPtr document;
    document = xmlReadMemory(content, length, "noname.xml", NULL, 0);
    if (document == nullptr)
    {
        throw std::runtime_error("Failed to parse XML");
    }
    xmlNode *rootElement{nullptr};
    rootElement = xmlDocGetRootElement(document); 
    auto streamListNode = ::findNode(rootElement, "StreamList"); 
    if (streamListNode == nullptr)
    {
        spdlog::warn("Could not find stream list");
    }
    else
    {
        xmlNode *childNode{nullptr};
        childNode = streamListNode->xmlChildrenNode;
        while (childNode != nullptr)
        {
            auto nodeName
                = xmlGetProp(childNode,
                             reinterpret_cast<const xmlChar *> ("Name"));
            if (nodeName)
            {
                auto stringLength = static_cast<size_t> (xmlStrlen(nodeName));
                auto stringContent = reinterpret_cast<const char *> (nodeName);
                std::string streamName{stringContent, stringLength};
                if (!result.contains(streamName))
                {
                    //std::cout << streamName << std::endl;
                    result.insert(streamName);
                }
                xmlFree(nodeName);
            }
/*
            for (xmlAttrPtr attribute = childNode->properties;
                 attribute != nullptr;
                 attribute = attribute->next)
            {
            }
            xmlChar *streamName;
            streamName = xmlGetProp(childNode, reinterpret_cast<const xmlChar *> ("Stream"));
std::cout << streamName << std::endl;
            if (streamName){xmlFree(streamName);}
*/
            // Update
            childNode = childNode->next;
        } 
        
    }
    xmlFreeDoc(document);
    return result;
}

/*
std::chrono::seconds now()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds> (now.time_since_epoch());
}
*/

using namespace MLReview::WaveServer;

class DataLink::DataLinkImpl
{
public:
    ~DataLinkImpl()
    {
        destroyDataLinkClient();
    }
    void createClient()
    {
        auto urlCopy = mURL;
        auto clientNameCopy = mClientName;
        mDataLinkClient = dl_newdlcp(urlCopy.data(),
                                     clientNameCopy.data());
        //mDataLinkClient->iotimeout = mTimeOut.count();
        mDataLinkClient->keepalive = mTimeOut.count();
    }
    void connect()
    {
        if (!mDataLinkClient){createClient();}
        disconnect();
        spdlog::debug("Connecting to DataLink server at " + mURL);
        if (dl_connect(mDataLinkClient) < 0)
        {
            if (mDataLinkClient){dl_freedlcp(mDataLinkClient);}
            throw std::runtime_error("Failed to connect to DataLink client at "
                                   + mClientName + " at " + mURL);
        }
        spdlog::debug("Connected to DataLink server!");
    }
    void destroyDataLinkClient()
    {
        disconnect();
        if (mDataLinkClient){dl_freedlcp(mDataLinkClient);}
        mDataLinkClient = nullptr;
    } 
    [[nodiscard]] bool isConnected() const noexcept
    {
        if (mDataLinkClient)
        {
            if (mDataLinkClient->link !=-1){return true;}
        }
        return false;
    }
    /// @brief Terminates the connection
    void disconnect()
    {
        if (isConnected())
        {
            spdlog::debug("Disconnecting...");
            dl_disconnect(mDataLinkClient);
        }
    }
    void createStreamList()
    {
        mStreamList.clear();
        connect();
        char *infoBuffer{nullptr};
        auto informationSize
            = dl_getinfo(mDataLinkClient, "STREAMS",
                         nullptr,
                         &infoBuffer, 0);
        if (informationSize > 0)
        {
            try
            {
                mStreamList = ::parser(infoBuffer, informationSize);
                spdlog::info("Found " + std::to_string(mStreamList.size())
                           + " streams");
            }
            catch (const std::exception &e)
            {
                spdlog::warn("Failed to create channel list");
            }
            if (infoBuffer){free(infoBuffer);}
        }
        else
        {
            if (infoBuffer){free(infoBuffer);}
        }
        disconnect();
    }
//private:
    DLCP *mDataLinkClient{nullptr};
    std::set<std::string> mStreamList;
    std::string mClientName{"daliClient"};
    std::string mURL;
    std::array<char, MAXPACKETSIZE> mBuffer;
    std::chrono::seconds mTimeOut{1};
};

/*
/// Constructor
DataLink::DataLink() :
    pImpl(std::make_unique<DataLinkImpl> ())
{
}
*/

/// Constructor
DataLink::DataLink(const std::string &url,
                   const std::string &clientName) :
    pImpl(std::make_unique<DataLinkImpl> ())
{
    if (url.empty()){throw std::invalid_argument("URL is empty");}
    if (clientName.empty()){throw std::invalid_argument("clientName is empty");}
    pImpl->mClientName = clientName;
    pImpl->mURL = url;
    pImpl->createStreamList();
}

Waveform DataLink::getData(const Request &request) const
{
    Waveform result;
    if (!request.haveNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!request.haveStation())
    {
        throw std::invalid_argument("Station not set");
    }
    if (!request.haveChannel())
    {
        throw std::invalid_argument("Channel not set");
    }
    if (!request.haveStartAndEndTime())
    {
        throw std::invalid_argument("Start and end time not set");
    }
    std::string locationCode;
    if (request.haveLocationCode())
    {
        locationCode = request.getLocationCode();
        if (locationCode == "--"){locationCode = "";}
    }
    auto queryString = request.getNetwork()
                     + "_"
                     + request.getStation()
                     + "_"
                     + locationCode
                     + "_"
                     + request.getChannel()
                     + "/MSEED";
    if (!pImpl->mStreamList.contains(queryString))
    {
        spdlog::warn("Stream: " + queryString
                   + " not in dataLink server");
        return result;
    }
    pImpl->destroyDataLinkClient();
    pImpl->connect();
    if (!isConnected())
    {
        throw std::runtime_error("No connection");
    }
    // Start the largest expected packet time before
    spdlog::debug("Querying data for " + queryString);
    dltime_t startTime
        = static_cast<dltime_t> (request.getStartTime().count() - 10000000);
    dltime_t endTime = static_cast<dltime_t> (request.getEndTime().count());
    auto returnCode = dl_match(pImpl->mDataLinkClient, queryString.data());  // TODO to reset set to null
    if (returnCode < 0)
    {
        throw std::runtime_error("Failed to set match pattern: " + queryString);
    }
    returnCode = dl_position_after(pImpl->mDataLinkClient, startTime);
    if (returnCode < 0)
    {   
        throw std::runtime_error("Failed to position client");
    }   
    DLPacket dataLinkPacket;
    int8_t endFlag{0};
    std::fill(pImpl->mBuffer.begin(), pImpl->mBuffer.end(), '\0');
    std::vector<char> packetData;
    int nPacketsFromDataLink{0};
    while (true)
    {
        returnCode = dl_collect(pImpl->mDataLinkClient,
                                &dataLinkPacket,
                                pImpl->mBuffer.data(),
                                pImpl->mBuffer.size(),
                                endFlag);
        if (returnCode == DLPACKET)
        {
            spdlog::debug("Packet received!");
            if (dataLinkPacket.datasize > 0)
            {
                if (dataLinkPacket.datastart <= endTime &&
                    dataLinkPacket.dataend >= startTime)
                {
                    nPacketsFromDataLink = nPacketsFromDataLink + 1;
                    packetData.insert(packetData.end(),
                                      pImpl->mBuffer.begin(),
                                      pImpl->mBuffer.begin()
                                    + dataLinkPacket.datasize);
                }
            }
            if (dataLinkPacket.dataend >= endTime){break;}
        }
        else if (returnCode == DLENDED)
        {
            spdlog::warn("Connection terminated for " + queryString); 
            break;
        }
        else if (returnCode == DLNOPACKET)
        {
            spdlog::debug("No packet received for non-blocking request");
            break;
        }
        else
        {
            spdlog::debug("Error in dl_collect: "
                        + std::to_string(returnCode));
            break;
        }
    }
    // Immediately terminate.  This does not appear to be happy when re-entrant.
    pImpl->destroyDataLinkClient();
    spdlog::debug("Read " + std::to_string(nPacketsFromDataLink)
                + " packets from data link");
    try
    {
        auto waveform = ::unpack(packetData.data(), packetData.size()); 
        //std::cout << waveform.getStartAndEndTime().first << " " << waveform.getStartAndEndTime().second << std::endl;
        waveform.mergeSegments();
        result = std::move(waveform);
        if (result.getNumberOfSegments() > 0)
        {
             spdlog::debug("Found data for " + queryString);
        } 
        //std::cout << waveform.getStartAndEndTime().first << " " << waveform.getStartAndEndTime().second << std::endl;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to create waveform; failed with: "
                               + std::string {e.what()});
    }
    return result;
}

/// Connected?
bool DataLink::isConnected() const noexcept
{
    return pImpl->isConnected();
}

/// Client type
std::string DataLink::getType() const noexcept
{
    return TYPE;
}

/// Destructor
DataLink::~DataLink() = default;
