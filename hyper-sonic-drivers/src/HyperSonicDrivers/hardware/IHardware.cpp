
#include <HyperSonicDrivers/hardware/IHardware.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <cassert>

namespace HyperSonicDrivers::hardware
{
IHardware::IHardware(const std::shared_ptr<audio::IMixer>& mixer) : m_mixer(mixer)
{
    if (m_mixer == nullptr)
    {
        utils::throwLogC<std::runtime_error>("mixer is null");
    }

    m_output_rate = m_mixer->freq;
}

IHardware::~IHardware()
{
    stop();
}

void IHardware::start(
    const std::shared_ptr<TimerCallBack>& callback,
    const audio::mixer::eChannelGroup     group,
    const uint8_t                         volume,
    const uint8_t                         pan,
    const int                             timerFrequency)
{
    if (m_channelId.has_value())
        stop();

    m_callback = callback;
    startCallback(group, volume, pan, timerFrequency);
}

void IHardware::stop()
{
    stopCallback();
    m_callback.reset();
}

uint32_t IHardware::setCallbackFrequency(const int timerFrequency)
{
    assert(timerFrequency != 0);

    const int d = getOutputRate() / timerFrequency;
    const int r = getOutputRate() % timerFrequency;

    // This is equivalent to (getRate() << FIXP_SHIFT) / BASE_FREQ
    // but less prone to arithmetic overflow.
    return (d << FIXP_SHIFT) + (r << FIXP_SHIFT) / timerFrequency;
}

void IHardware::stopCallback()
{
    if (m_channelId.has_value())
    {
        m_mixer->reset(m_channelId.value());
        m_channelId = std::nullopt;
    }
}

void IHardware::callCallback()
{
    if (m_callback != nullptr)
        (*m_callback)();
}
}    // namespace HyperSonicDrivers::hardware
