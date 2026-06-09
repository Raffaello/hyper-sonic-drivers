#include <format>
#include <HyperSonicDrivers/hardware/opl/OPL.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/audio/streams/EmulatedStream.hpp>

namespace HyperSonicDrivers::hardware::opl
{
using utils::logE;

OPL::OPL(const std::shared_ptr<audio::IMixer>& mixer, const OplType type) : IHardware(mixer),
                                                                            type(type)
{
}

void OPL::start(
    const std::shared_ptr<TimerCallBack>& callback,
    const audio::mixer::eChannelGroup     group,
    const uint8_t                         volume,
    const uint8_t                         pan,
    const int                             timerFrequency)
{
    IHardware::start(callback, group, volume, pan, timerFrequency);
}

void OPL::startCallback(
    const audio::mixer::eChannelGroup group,
    const uint8_t                     volume,
    const uint8_t                     pan,
    const int                         timerFrequency)
{
    setAudioStream(std::make_shared<audio::streams::EmulatedStream>(
        this,
        isStereo(),
        m_mixer->freq,
        setCallbackFrequency(timerFrequency)));

    m_channelId = m_mixer->play(
        group,
        getAudioStream(),
        volume,
        pan);

    if (!m_channelId.has_value())
    {
        utils::logC("can't start opl playback");
    }
}
}    // namespace HyperSonicDrivers::hardware::opl
