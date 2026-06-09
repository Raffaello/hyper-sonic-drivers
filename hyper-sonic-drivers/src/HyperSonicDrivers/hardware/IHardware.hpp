#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <optional>
#include <HyperSonicDrivers/audio/IMixer.hpp>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/audio/IAudioStream.hpp>

namespace HyperSonicDrivers::audio
{
class IRenderer;

namespace streams
{
class EmulatedStream;
}
}    // namespace HyperSonicDrivers::audio

namespace HyperSonicDrivers::hardware
{
typedef std::function<void()> TimerCallBack;

constexpr int FIXP_SHIFT = 16;

class IHardware
{
    friend audio::streams::EmulatedStream;
    friend audio::IRenderer;

public:
    explicit IHardware(const std::shared_ptr<audio::IMixer>& mixer);
    virtual ~IHardware();

    inline bool isInit() const noexcept { return m_init; }

    virtual bool isStereo() const noexcept = 0;
    virtual bool init()                    = 0;
    virtual void reset()                   = 0;

    virtual void start(
        const std::shared_ptr<TimerCallBack>& callback,
        const audio::mixer::eChannelGroup     group,
        const uint8_t                         volume,
        const uint8_t                         pan,
        const int                             timerFrequency);

    void stop();

    virtual uint32_t setCallbackFrequency(const int timerFrequency);

    inline std::shared_ptr<audio::IMixer> getMixer() const noexcept { return m_mixer; };

    inline std::optional<uint8_t> getChannelId() const noexcept { return m_channelId; };

    inline uint32_t getOutputRate() const noexcept { return m_output_rate; };

protected:
    virtual void startCallback(
        const audio::mixer::eChannelGroup group,
        const uint8_t                     volume,
        const uint8_t                     pan,
        const int                         timerFrequency) = 0;

    void stopCallback();
    void callCallback();

    /**
     * get the emulated generated audio stream
     **/
    inline std::shared_ptr<audio::IAudioStream> getAudioStream() const noexcept { return m_stream; };

    inline void setAudioStream(const std::shared_ptr<audio::IAudioStream>& stream) noexcept { m_stream = stream; }

    /**
     * Read up to 'length' samples.
     *
     * Data will be in native endianess, 16 bit per sample, signed.
     * For stereo OPL, buffer will be filled with interleaved
     * left and right channel samples, starting with a left sample.
     * Furthermore, the samples in the left and right are summed up.
     * So if you request 4 samples from a stereo OPL, you will get
     * a total of two left channel and two right channel samples.
     */
    virtual void generateSamples(int16_t* buffer, const size_t length) noexcept = 0;

    bool                           m_init  = false;
    std::shared_ptr<audio::IMixer> m_mixer = nullptr;
    std::optional<uint8_t>         m_channelId;
    uint32_t                       m_output_rate;

private:
    std::shared_ptr<TimerCallBack>       m_callback = nullptr;
    std::shared_ptr<audio::IAudioStream> m_stream   = nullptr;
};
}    // namespace HyperSonicDrivers::hardware
