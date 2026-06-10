#pragma once

#include <cstdint>
#include <memory>
#include <filesystem>
#include <optional>
#include <mt32emu/c_interface/cpp_interface.h>
#include <HyperSonicDrivers/audio/IMixer.hpp>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/hardware/IHardware.hpp>
#include <HyperSonicDrivers/hardware/mt32/MT32ReportHandler.hpp>

namespace HyperSonicDrivers::devices
{
class MT32;
}

namespace HyperSonicDrivers::hardware::mt32
{
constexpr int      mt32_frequency_internal = 32000;    // 32 KHz
constexpr int      mt32_frequency          = 250;
constexpr uint32_t mt32_sysex_addr_LCD     = 0x80000;

class MT32 : public IHardware
{
public:
    MT32(const std::filesystem::path& control_rom, const std::filesystem::path& pcm_rom, const std::shared_ptr<audio::IMixer>& mixer);
    ~MT32() override;

    inline bool isStereo() const noexcept override { return true; };

    bool init() override;
    void reset() override;

    void start(
        const std::shared_ptr<TimerCallBack>& callback,
        const audio::mixer::eChannelGroup     group          = audio::mixer::eChannelGroup::Music,
        const uint8_t                         volume         = 255,
        const uint8_t                         pan            = 0,
        const int                             timerFrequency = mt32_frequency) override;

    inline MT32Emu::Service& getService() noexcept { return m_service; };

    void sysEx(const uint32_t addr, const uint8_t* data, const uint32_t dataSize);

protected:
    void startCallback(
        const audio::mixer::eChannelGroup group,
        const uint8_t                     volume,
        const uint8_t                     pan,
        const int                         timerFrequency) override;

    void generateSamples(int16_t* buffer, const size_t length) noexcept override;

private:
    MT32Emu::Service  m_service;
    MT32ReportHandler m_rh;
};
}    // namespace HyperSonicDrivers::hardware::mt32
