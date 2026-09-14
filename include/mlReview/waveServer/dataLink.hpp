#ifndef MLREVIEW_WAVE_SERVER_DATA_LINK_HPP
#define MLREVIEW_WAVE_SERVER_DATA_LINK_HPP
#include <memory>
#include <mlReview/waveServer/client.hpp>
namespace MLReview::WaveServer
{
/// @class DataLink "dataLink.hpp" "mlReview/waveServer/dataLink.hpp"
/// @brief This pulls data from a DataLink (ring)server.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class DataLink final : public IClient
{
public:
    DataLink() = delete;
    /// @brief Constructor. 
    /// @param[in] url         The URL of the DataLink server.
    /// @param[in] clientName  The name of this client.
    explicit DataLink(const std::string &url,
                      const std::string &clientName = "DataLinkClient");

    /// @result True indicates the client is connected.
    [[nodiscard]] bool isConnected() const noexcept;
    /// @brief Fetches the data from the DataLink client.
    [[nodiscard]] Waveform getData(const Request &request) const override final;
    /// @result The client type which is DataLink
    [[nodiscard]] std::string getType() const noexcept final;
    /// @brief Destructor.
    ~DataLink() override;
 
    DataLink(const DataLink &) = delete;
    DataLink& operator=(const DataLink &) = delete;
private:
    class DataLinkImpl;
    std::unique_ptr<DataLinkImpl> pImpl;    
};
}
#endif
