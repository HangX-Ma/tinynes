#ifndef TINYNES_VSOUND_H
#define TINYNES_VSOUND_H

#include <memory>
#include <utility>
#include <vector>
#include <SFML/Config.hpp>
#include <SFML/Audio/SoundStream.hpp>

namespace tn
{

class Bus;
class VSound : public sf::SoundStream
{
public:
    void init(sf::Int16 length, sf::Uint32 channel_count, sf::Uint32 sample_rate,
              std::shared_ptr<Bus> nes);

private:
    bool onGetData(Chunk &data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    std::shared_ptr<Bus> nes_{nullptr};
    std::vector<sf::Int16> samples_;
    std::size_t current_sample_;

    sf::Int16 max_sample_{32767};
};

} // namespace tn

#endif