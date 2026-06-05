#pragma once

#include <HyperSonicDrivers/drivers/IAudioDriver.hpp>
#include <HyperSonicDrivers/devices/Opl.hpp>
#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <HyperSonicDrivers/hardware/opl/OPL.hpp>

#include <memory>
#include <cstdint>
#include <mutex>

namespace HyperSonicDrivers::drivers::opl
{

class IMFDriver : public IAudioDriver
{
private:
    std::shared_ptr<hardware::opl::OPL> m_opl;
    std::shared_ptr<files::IMFFile>     m_imf_file;
    uint16_t                            m_delay_ticks = 0;
    bool                                m_isPlaying   = false;
    uint32_t                            m_pos         = 0;

    mutable std::mutex m_mutex;

    void onCallback_();

public:
    explicit IMFDriver(
        const std::shared_ptr<devices::Opl>& opl,
        const audio::mixer::eChannelGroup    group,
        const uint8_t                        volume = 255,
        const uint8_t                        pan    = 0);

    ~IMFDriver() override;

    void setIMFFile(const std::shared_ptr<files::IMFFile>& imf_file);

    void play(const uint16_t track) noexcept override;
    void stop() noexcept override;
    bool isPlaying() const noexcept override;
};

}    // namespace HyperSonicDrivers::drivers::opl
