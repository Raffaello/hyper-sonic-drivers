#include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

#include <cassert>

namespace HyperSonicDrivers::audio::rtaudio
{

using utils::logE;
using utils::logW;
using utils::logI;

Mixer::Mixer(const uint8_t  max_channels,
             const uint32_t freq,
             const uint16_t buffer_size)
    : MixerBase(max_channels, freq, buffer_size)
{
}

Mixer::~Mixer()
{
    shutdown();
}

void Mixer::suspend() noexcept
{
    if (m_audio.isStreamRunning())
    {
        if (m_audio.stopStream() != RTAUDIO_NO_ERROR)
            logE("unable to stop audio stream");
    }
}

void Mixer::resume() noexcept
{
    if (!m_audio.isStreamRunning())
    {
        if (m_audio.startStream() != RTAUDIO_NO_ERROR)
            logE(std::format("unable to start audio stream"));
    }
}

bool Mixer::onInit_()
{
    return init_(rtAudioCallback_, this);
}

void Mixer::onShutdown_()
{
    if (m_audio.isStreamRunning())
    {
        if (m_audio.stopStream() != RTAUDIO_NO_ERROR)
            logE("unable to stop audio stream");
    }

    if (m_audio.isStreamOpen())
        m_audio.closeStream();
}

bool Mixer::init_(RtAudioCallback callback, void* userdata)
{
    if (m_audio.getDeviceIds().size() < 1)
    {
        logE("No audio device found!");
        return false;
    }

    RtAudio::StreamParameters params{
        .deviceId     = m_audio.getDefaultOutputDevice(),
        .nChannels    = 2,
        .firstChannel = 0,
    };

    auto info = m_audio.getDeviceInfo(params.deviceId);
    logI(std::format("using deviceId={}, {}, {}, {}", params.deviceId, info.name, info.outputChannels, info.currentSampleRate));

    RtAudio::StreamOptions options{
        .flags = RTAUDIO_SCHEDULE_REALTIME};

    unsigned int bufFrames = buffer_size / sizeof(int16_t) / 2;
    if (m_audio.openStream(&params, nullptr, RTAUDIO_SINT16, freq, &bufFrames, callback, userdata, &options) != RTAUDIO_NO_ERROR)
    {
        logE(std::format("unable to open stream: {}", m_audio.getErrorText()));
        return false;
    }

    return true;
}

void Mixer::callback_(int16_t* buf, int num_samples)
{
    memset(buf, 0, num_samples * sizeof(int16_t) * 2);    // stereo
    for (const auto& ch : m_channels)
        ch->mix(buf, num_samples);
}

int Mixer::rtAudioCallback_(void*               outputBuffer,
                            void*               inputBuffer,
                            unsigned int        nFrames,
                            double              streamTime,
                            RtAudioStreamStatus status,
                            void*               userData)
{
    assert(userData != nullptr);

    if (status != 0)
        logW(std::format("RtAudio callback status: {}", status));

    Mixer*   mixer = static_cast<Mixer*>(userData);
    int16_t* buf   = static_cast<int16_t*>(outputBuffer);

    mixer->callback_(buf, nFrames);
    return 0;
}

}    // namespace HyperSonicDrivers::audio::rtaudio
