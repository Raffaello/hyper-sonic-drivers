#include <HyperSonicDrivers/drivers/midi/mt32/MT32Driver.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::drivers::midi::mt32
{
constexpr void dead_end()
{
    utils::throwLogC<std::invalid_argument>("can't call this method");
}

MT32Driver::MT32Driver(const std::shared_ptr<devices::MT32>& mt32)
{
    if (mt32 == nullptr)
        utils::throwLogC<std::runtime_error>("MT32 device is null");

    m_mt32 = mt32->getMt32();
}

MT32Driver::~MT32Driver()
{
    // TODO: this can be done in the parent class using IHardware pointer.
    MT32Driver::close();
}

bool MT32Driver::open(const audio::mixer::eChannelGroup group, const uint8_t volume, const uint8_t pan)
{
    if (isOpen())
        return true;

    if (!m_mt32->init())
        return false;

    // TODO: mostly the same as for OPL
    hardware::TimerCallBack cb = std::bind_front(&MT32Driver::callback_, this);
    auto                    p  = std::make_shared<hardware::TimerCallBack>(cb);
    m_mt32->start(p, group, volume, pan, MT32Driver::CLOCK_HZ);

    m_isOpen = true;
    return true;
}

void MT32Driver::close()
{
    // TODO: this can be done in the parent class using IHardware pointer
    if (isOpen())
    {
        m_mt32->stop();
        m_isOpen = false;
    }
}

void MT32Driver::send(const uint32_t msg) noexcept
{
    // NOTE/TODO: the mt32Emu library could directly play midi
    // TODO: and it has other parameters that requires a tune up,
    //       that should be done in the device i guess..
    m_mt32->getService().playMsg(msg);
}

// void MT32Driver::onCallback() noexcept
// {
//     // TODO
// }

void MT32Driver::noteOff(const uint8_t chan, const uint8_t note) noexcept
{
    dead_end();
}

void MT32Driver::noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept
{
    dead_end();
}

void MT32Driver::controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::programChange(const uint8_t chan, const uint8_t program) noexcept
{
    dead_end();
}

void MT32Driver::pitchBend(const uint8_t chan, const uint16_t bend) noexcept
{
    dead_end();
}

void MT32Driver::sysEx(const uint8_t* msg, uint16_t length) noexcept
{
    m_mt32->getService().playSysex(msg, length);
}

void MT32Driver::ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_volume(const uint8_t chan, const uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_panPosition(const uint8_t chan, uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_sustain(const uint8_t chan, uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_reverb(const uint8_t chan, uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_chorus(const uint8_t chan, uint8_t value) noexcept
{
    dead_end();
}

void MT32Driver::ctrl_allNotesOff() noexcept
{
    dead_end();
}
}    // namespace HyperSonicDrivers::drivers::midi::mt32
