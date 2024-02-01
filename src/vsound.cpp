#include "tinynes/vsound.h"
#include "tinynes/bus.h"
#include <SFML/Config.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

namespace tn
{

void VSound::init(sf::Int16 length, sf::Uint32 channel_count, sf::Uint32 sample_rate,
                  std::shared_ptr<Bus> nes)
{
    samples_.resize(length, 0);
    current_sample_ = 0;
    initialize(channel_count, sample_rate);

    nes_ = std::move(nes);
    nes_->setAudioSampleFrequency(sample_rate);

    spdlog::info("Virtual Sound: channel num {}, sample rate {}, buffer size {}", getChannelCount(),
                 getSampleRate(), samples_.size());
}

bool VSound::onGetData(Chunk &data)
{
    auto get_mixer_sample = [&]()
    {
        while (!nes_->clock()) {
        };
        return static_cast<float>(nes_->getAudioSample());
    };

    auto clip = [](float sample, float max)
    {
        if (sample >= 0.0) {
            return std::fmin(sample, max);
        }
        return std::fmax(sample, -max);
    };

    int len = samples_.size();
    for (int i = 0; i < len; ++i) {
        samples_[i] = 0;
    }

    data.samples = &samples_[current_sample_];

    for (size_t n = 0; n < samples_.size(); n += getChannelCount()) {
        for (uint32_t c = 0; c < getChannelCount(); c += 1) {
            samples_[n + c] = clip(get_mixer_sample(), 1.0) * max_sample_;
        }
    }
    data.sampleCount = samples_.size();
    current_sample_ = 0;

    return true;
}

void VSound::onSeek(sf::Time timeOffset)
{
    current_sample_
        = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
}

} // namespace tn