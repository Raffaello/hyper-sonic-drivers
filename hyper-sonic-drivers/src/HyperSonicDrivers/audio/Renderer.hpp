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
    Renderer(const uint32_t freq, const uint16_t buffer_size, const uint8_t max_channels = 1);
    ~Renderer() override = default;

    void openOutputFile(const std::filesystem::path& path) override;
    void closeOutputFile() noexcept override;

    using IRenderer::renderBuffer;
    using IRenderer::renderFlush;
    using IRenderer::renderBufferFlush;

    void renderBuffer(IAudioStream* stream) override;
    bool renderFlush(IAudioStream* stream) override;
    bool renderBufferFlush(IAudioStream* stream, drivers::IAudioDriver& drv, const uint8_t track) override;
};
}    // namespace HyperSonicDrivers::audio
