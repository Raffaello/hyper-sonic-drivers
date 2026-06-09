#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>
#include <HyperSonicDrivers/audio/IAudioStream.hpp>
#include <HyperSonicDrivers/hardware/opl/OPL.hpp>
#include <HyperSonicDrivers/devices/IDevice.hpp>
#include <HyperSonicDrivers/drivers/IAudioDriver.hpp>
#include <HyperSonicDrivers/files/WAVFile.hpp>

namespace HyperSonicDrivers::audio
{
class IRenderer
{
public:
    IRenderer()          = default;
    virtual ~IRenderer() = default;

    virtual void openOutputFile(const std::filesystem::path& path) = 0;
    virtual void closeOutputFile() noexcept                        = 0;

    virtual void renderBuffer(IAudioStream* stream) = 0;

    inline void renderBuffer(const std::shared_ptr<devices::IDevice>& device) { renderBuffer(device->getHardware()->getAudioStream().get()); };

    /**
     * It flushes the AudioStream until it detects the silence.
     * It returns false if it reached max iterations but it didn't flush it completely.
     * It returns true when silence is detected or nothing more to read from the stream
     **/
    virtual bool renderFlush(IAudioStream* stream) = 0;

    inline bool renderFlush(const std::shared_ptr<devices::IDevice>& device) { return renderFlush(device->getHardware()->getAudioStream().get()); };

    /**
     * this is doing the render until the drv is play the track and call the render flush after all
     * it returns the renderFlush returned value
     **/
    virtual bool renderBufferFlush(IAudioStream* stream, drivers::IAudioDriver& drv, const uint8_t track) = 0;

    inline bool renderBufferFlush(const std::shared_ptr<devices::IDevice>& device, drivers::IAudioDriver& drv, const uint8_t track) { return renderBufferFlush(device->getHardware()->getAudioStream().get(), drv, track); };

protected:
    std::unique_ptr<files::WAVFile> m_out;
    std::vector<int16_t>            m_buf;
};
}    // namespace HyperSonicDrivers::audio
