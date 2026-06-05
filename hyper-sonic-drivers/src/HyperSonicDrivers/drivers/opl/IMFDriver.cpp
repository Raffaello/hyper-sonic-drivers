#include <HyperSonicDrivers/drivers/opl/IMFDriver.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>

namespace HyperSonicDrivers::drivers::opl
{
using HyperSonicDrivers::utils::throwLogE;

IMFDriver::IMFDriver(
    const std::shared_ptr<devices::Opl>& opl,
    const audio::mixer::eChannelGroup    group,
    const uint8_t                        volume,
    const uint8_t                        pan) : IAudioDriver(opl),
                         m_opl(opl->getOpl())
{
    // NOTE: it must acquire it due to opl->start setting the callback
    if (!m_device->acquire(this))
        throwLogE<std::runtime_error>("Device is already in used by another driver or can't be init");

    // m_opl->start(
    //     p,
    //     group,
    //     volume,
    //     pan,
    //     callbacks_per_second);

    // stopAllChannels();
    // initDriver_();
    // setOplMusicVolume(255);
    // setOplSfxVolume(255);
    m_opl->start(nullptr);
}

IMFDriver::~IMFDriver()
{
    m_opl->stop();
    m_device->release(this);
}

void IMFDriver::setIMFFile(const std::shared_ptr<files::IMFFile>& imf_file)
{
    m_imf_file = imf_file;
}

void IMFDriver::play([[maybe_unused]] const uint16_t track) noexcept
{
    for (const auto& packet : m_imf_file->data())
    {
        m_opl->writeReg(packet.reg, packet.val);
        if (packet.delay_ticks > 0)
            utils::delayMicro(packet.delay_ticks * 700 * 2);    // 700 Hz timing
    }
}

void IMFDriver::stop() noexcept
{
}

bool IMFDriver::isPlaying() const noexcept
{
    return false;
}

}    // namespace HyperSonicDrivers::drivers::opl
