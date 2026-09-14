#ifndef MLREVIEW_WAVE_SERVER_MULTI_CLIENT_HPP
#define MLREVIEW_WAVE_SERVER_MULTI_CLIENT_HPP
#include <memory>
#include <mlReview/waveServer/client.hpp>
namespace MLReview::WaveServer
{
 class Waveform;
}
namespace MLReview::WaveServer
{
class MultiClient final : public IClient
{
public:
    MultiClient();
    void insert(std::unique_ptr<IClient> &&client, int priority);
    [[nodiscard]] std::vector<Waveform> getData(const std::vector<Request> &requests) const override final;
    [[nodiscard]] Waveform getData(const Request &request) const override final;
    [[nodiscard]] std::string getType() const noexcept override final;
    ~MultiClient() override;
private:
    class MultiClientImpl;
    std::unique_ptr<MultiClientImpl> pImpl;
};
}
#endif
