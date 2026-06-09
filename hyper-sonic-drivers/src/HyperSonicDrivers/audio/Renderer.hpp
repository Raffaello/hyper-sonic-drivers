#pragma once

#include <cstdint>
#include <filesystem>
#include <HyperSonicDrivers/audio/IRenderer.hpp>
#include <HyperSonicDrivers/audio/IAudioStream.hpp>

namespace HyperSonicDrivers::audio
{
class Renderer : public IRenderer
{
public:
    Renderer(const uint16_t buffer_size);
    ~Renderer() override = default;

    void openOutputFile(const std::filesystem::path& path) override;
    void closeOutputFile() noexcept override;

    using IRenderer::renderBuffer;
    using IRenderer::renderFlush;
    using IRenderer::renderBufferFlush;

    void renderBuffer(IAudioStream* stream) override;
    bool renderFlush(IAudioStream* stream) override;
    bool renderBufferFlush(IAudioStream* stream, drivers::IAudioDriver& drv, const uint8_t track) override;

    const uint16_t m_buffer_size;
};
}    // namespace HyperSonicDrivers::audio
