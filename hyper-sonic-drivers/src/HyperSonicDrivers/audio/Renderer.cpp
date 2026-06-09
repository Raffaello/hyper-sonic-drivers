#include <HyperSonicDrivers/audio/Renderer.hpp>

#if HAS_SDL3

#include <HyperSonicDrivers/audio/sdl3/Mixer.hpp>
#define MIXER sdl3::Mixer
#elif HAS_SDL2
#include <HyperSonicDrivers/audio/sdl2/Mixer.hpp>
#define MIXER sdl2::Mixer
#else
#include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
#define MIXER rtaudio::Mixer
#endif

#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <algorithm>
#include <ranges>

namespace HyperSonicDrivers::audio
{
// MaxRendererFlushIterations defines the maximum number of iterations
// to attempt flushing the renderer buffer to prevent infinite loops.
constexpr int MaxRendererFlushIterations = 1000;

Renderer::Renderer(const uint32_t freq, const uint16_t buffer_size, const uint8_t max_channels)
{
    m_buffer_size = buffer_size;
}

void Renderer::openOutputFile(const std::filesystem::path& path)
{
    m_out = std::make_unique<files::WAVFile>(path.string(), audio::mixer::eChannelGroup::Unknown, false);
    m_buf.resize(0);
}

void Renderer::closeOutputFile() noexcept
{
    m_out.reset();
}

void Renderer::renderBuffer(IAudioStream* stream)
{
    if (m_buf.empty())
    {
        m_out->save_prepare(stream->getRate(), stream->isStereo());
        m_buf.resize(m_buffer_size);
    }

    const size_t read = stream->readBuffer(m_buf.data(), m_buf.size());
    m_out->save_streaming(m_buf.data(), read);
}

bool Renderer::renderFlush(IAudioStream* stream)
{
    // safety check
    if (m_buf.empty())
    {
        m_out->save_prepare(stream->getRate(), stream->isStereo());
        m_buf.resize(m_buffer_size);
    }

    for (int i = 0; i < MaxRendererFlushIterations; i++)
    {
        const size_t read = stream->readBuffer(m_buf.data(), m_buf.size());
        if (read == 0)
            return true;

        // check if it is all silence...
        if (std::ranges::all_of(m_buf, [](const int16_t sample) { return sample == 0; }))
            return true;

        m_out->save_streaming(m_buf.data(), read);
    }

    return false;
}

bool Renderer::renderBufferFlush(IAudioStream* stream, drivers::IAudioDriver& drv, const uint8_t track)
{
    drv.play(track);
    while (drv.isPlaying())
        renderBuffer(stream);

    return renderFlush(stream);
}
}    // namespace HyperSonicDrivers::audio
