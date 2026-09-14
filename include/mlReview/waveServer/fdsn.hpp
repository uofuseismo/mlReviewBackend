#ifndef MLREVIEW_WAVE_SERVER_FDSN_HPP
#define MLREVIEW_WAVE_SERVER_FDSN_HPP
#include <mlReview/waveServer/client.hpp>
#include <memory>
namespace MLReview::WaveServer
{
/// @class FDSN "fdsn.hpp" "mlReview/waveServer/fdsn.hpp"
/// @brief Requests waveforms from a Federation of Digital Seismograph
///        Network-esque API.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class FDSN final : public IClient
{
public:
    FDSN(); 
    explicit FDSN(const std::string &url);

    [[nodiscard]] Waveform getData(const Request &request) const final;
    /// @result The client type which is FDSN.
    [[nodiscard]] std::string getType() const noexcept final;
    /// @brief Destrutor.
    ~FDSN() override;
private:
    class FDSNImpl;
    std::unique_ptr<FDSNImpl> pImpl;
};
}
#endif
