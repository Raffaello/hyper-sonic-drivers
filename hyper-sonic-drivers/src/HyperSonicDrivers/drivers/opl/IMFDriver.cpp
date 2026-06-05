#include <HyperSonicDrivers/drivers/opl/IMFDriver.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>

namespace HyperSonicDrivers::drivers::opl
{
using HyperSonicDrivers::utils::throwLogE;

constexpr int CALLBACKS_PER_SECONDS = 700;    // 700 hz

void IMFDriver::onCallback_()
{
    if (!m_isPlaying)
        return;

    if (m_delay_ticks > 0)
    {
        --m_delay_ticks;
        return;
    }

    if (m_pos >= m_imf_file->data().size())
    {
        m_isPlaying = false;
        return;
    }

    const auto& packet = m_imf_file->data()[m_pos];
    m_opl->writeReg(packet.reg, packet.val);
    m_delay_ticks = packet.delay_ticks;
    ++m_pos;
}

IMFDriver::IMFDriver(
    const std::shared_ptr<devices::Opl>& opl,
    const audio::mixer::eChannelGroup    group,
    const uint8_t                        volume,
    const uint8_t                        pan) : IAudioDriver(opl),
                         m_opl(opl->getOpl())
{
    hardware::TimerCallBack cb          = std::bind(&IMFDriver::onCallback_, this);
    auto                    pOnCallback = std::make_shared<hardware::TimerCallBack>(cb);

    // NOTE: it must acquire it due to opl->start setting the callback
    if (!m_device->acquire(this))
        throwLogE<std::runtime_error>("Device is already in used by another driver or can't be init");

    m_opl->start(
        pOnCallback,
        group,
        volume,
        pan,
        CALLBACKS_PER_SECONDS);
}

IMFDriver::~IMFDriver()
{
    m_opl->stop();
    m_device->release(this);
}

void IMFDriver::setIMFFile(const std::shared_ptr<files::IMFFile>& imf_file)
{
    stop();
    m_imf_file    = imf_file;
    m_delay_ticks = 0;
    m_pos         = 0;
}

void IMFDriver::play([[maybe_unused]] const uint16_t track) noexcept
{
    m_delay_ticks = 0;
    m_pos         = 0;
    m_isPlaying   = true;
}

void IMFDriver::stop() noexcept
{
    m_isPlaying = false;
}

bool IMFDriver::isPlaying() const noexcept
{
    return m_isPlaying;
}

}    // namespace HyperSonicDrivers::drivers::opl
