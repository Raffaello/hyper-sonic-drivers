#include <array>
#include <thread>
#include <format>
#include <HyperSonicDrivers/drivers/MIDDriver.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/drivers/midi/opl/OplDriver.hpp>
#include <HyperSonicDrivers/devices/Opl.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/MidiDriver_ADLIB.hpp>

#if HAS_MT32_EMU
#include <HyperSonicDrivers/drivers/midi/mt32/MT32Driver.hpp>
#endif

namespace HyperSonicDrivers::drivers
{
using utils::logC;
using utils::logD;
using utils::logI;
using utils::logT;
using utils::logW;

MIDDriver::MIDDriver(
    const std::shared_ptr<devices::IDevice>& device,
    const audio::mixer::eChannelGroup        group,
    const uint8_t                            volume,
    const uint8_t                            pan) : IAudioDriver(device),
                         m_group(group), m_volume(volume), m_pan(pan)
{
    // TODO: move the acquire logic where the callback is set
    // NOTE/TODO: this brings up the acquire should set up the callback too?
    // it will brings to store m_device into IMidiDriver and pass it in that constructor...
    // so not sure at the moment, but i think the driver should be responsible to acquire the hardware/device
    // when they are open, and release it when they are closed
    if (!m_device->acquire(this))
    {
        utils::throwLogE<std::runtime_error>("Device is already in used by another driver or can't be init");
    }

    // The internal Midi driver will start the device and set up the callback
    if (!resetBankOP2())
    {
        utils::throwLogE<std::runtime_error>("can't reset Midi driver");
    }
}

MIDDriver::~MIDDriver()
{
    MIDDriver::stop();
    m_device->release(this);
}

void MIDDriver::setMidi(const std::shared_ptr<audio::MIDI>& midi) noexcept
{
    using audio::midi::MIDI_FORMAT;

    if (midi->format == MIDI_FORMAT::SIMULTANEOUS_TRACK)
    {
        utils::throwLogE<std::runtime_error>("Can't support MIDI format 1 (SIMULTANEOUS_TRACK), must be converted to format 0 (SINGLE_TRACK)");
        return;
    }

    m_midi = midi;
}

bool MIDDriver::loadBankOP2(const std::shared_ptr<audio::opl::banks::OP2Bank>& op2Bank) noexcept
{
    if (m_device->type != devices::eDeviceType::Opl)
        return false;

    if (op2Bank == nullptr)
    {
        utils::logE("OP2Bank is nullptr");
        return false;
    }

    m_midiDriver.reset();
    auto opl_drv = std::make_unique<drivers::midi::opl::OplDriver>(std::dynamic_pointer_cast<devices::Opl>(m_device));
    opl_drv->setOP2Bank(op2Bank);
    m_midiDriver = std::move(opl_drv);
    return open_();
}

bool MIDDriver::resetBankOP2() noexcept
{
    switch (m_device->type)
    {
        using enum devices::eDeviceType;

    case Opl:
        m_midiDriver = std::make_unique<drivers::midi::scummvm::MidiDriver_ADLIB>(std::dynamic_pointer_cast<devices::Opl>(m_device));
        break;
#if HAS_MT32_EMU
    case Mt32:
        m_midiDriver = std::make_unique<drivers::midi::mt32::MT32Driver>(std::dynamic_pointer_cast<devices::MT32>(m_device));
        break;
#endif
    default:
        utils::throwLogC<std::invalid_argument>(std::format("unknown device type {:d}", static_cast<int>(m_device->type)));
        break;
    }

    return open_();
}

void MIDDriver::play(const uint16_t track) noexcept
{
    if (m_midi == nullptr)
        return;

    if (track >= m_midi->numTracks)
    {
        logW(std::format("track not available: {}", track));
        return;
    }

    // TODO: this could be to set the callback frequency?
    //       this block is doing nothing now.....
    if (m_midi->division & 0x8000)
    {
        // ticks per frame
        int smpte         = (m_midi->division & 0x7FFF) >> 8;
        int ticksPerFrame = m_midi->division & 0xFF;
        switch (smpte)
        {
        case -24:
        case -25:
        case -29:
        case -30:
            logW("SMPTE not implemented yet");
            break;
        default:
            logW(std::format("Division SMPTE not known = {}", smpte));
        }

        logD(std::format("Division: Ticks per frame = {}, {}", ticksPerFrame, smpte));
        logW("division ticks per frame not implemented yet");
    }
    else
    {
        // ticks per quarter note
        logD(std::format("Division: Ticks per quarter note = {}", m_midi->division & 0x7FFF));
    }

    stop();
    if (!m_device->acquire(this))
        return;

    m_midiDriver->play(m_midi->getTrack(track).getEvents(), m_midi->division);
}

void MIDDriver::stop() noexcept
{
    if (m_midiDriver != nullptr)
        m_midiDriver->stop();
}

void MIDDriver::pause() noexcept
{
    if (m_midiDriver != nullptr)
        m_midiDriver->pause();
}

void MIDDriver::resume() noexcept
{
    if (m_midiDriver != nullptr)
        m_midiDriver->resume();
}

bool MIDDriver::isPlaying() const noexcept
{
    return m_midiDriver != nullptr ? m_midiDriver->isPlaying() : false;
}

bool MIDDriver::isPaused() const noexcept
{
    return m_midiDriver != nullptr ? m_midiDriver->isPaused() : false;
}

bool MIDDriver::open_() noexcept
{
    if (!m_midiDriver->open(m_group, m_volume, m_pan))
    {
        utils::logE("can't open midi driver");
        return false;
    }

    return true;
}

}    // namespace HyperSonicDrivers::drivers
