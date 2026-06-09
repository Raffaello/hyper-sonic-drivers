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

constexpr uint32_t DEFAULT_MIDI_TEMPO = 500000;
constexpr int32_t  CLOCK_HZ           = 10'000;

constexpr uint32_t tempo_to_micros(const uint32_t tempo, const uint16_t division)
{
    // TODO: it can be integer division? test it.
    return static_cast<uint32_t>(static_cast<float>(tempo) / static_cast<float>(division));
}

inline uint32_t get_start_time()
{
    return utils::getMicro<uint32_t>();
}

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

    m_division  = m_midi->division & 0x7FFF;
    m_pEvents   = &m_midi->getTrack(track).getEvents();
    m_pos       = 0;
    m_paused    = false;
    m_isPlaying = true;
    // setTempo(DEFAULT_MIDI_TEMPO);    // 120 BPM;
    hardware::TimerCallBack cb = std::bind_front(&MIDDriver::onCallback_, this);
    m_midiDriver->setCallback(cb, CLOCK_HZ);    // 1Khz
    m_delta_step = CLOCK_HZ / 2;                // 1Khz / 2Hz ratio (2Hz=120 BPM)
}

void MIDDriver::stop() noexcept
{
    m_paused    = false;
    m_isPlaying = false;
}

void MIDDriver::pause() noexcept
{
    if (m_isPlaying)
        m_paused = true;
}

void MIDDriver::resume() noexcept
{
    if (m_isPlaying)
        m_paused = false;
}

bool MIDDriver::isPlaying() const noexcept
{
    return m_isPlaying;
}

bool MIDDriver::isPaused() const noexcept
{
    return m_paused;
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

void MIDDriver::onCallback_()
{
    using audio::midi::MIDI_EVENT_TYPES_HIGH;
    using audio::midi::MIDI_META_EVENT;
    using audio::midi::MIDI_META_EVENT_TYPES_LOW;
    using audio::midi::MIDI_META_EVENT_VAL;
    using audio::midi::TO_HIGH;
    using audio::midi::TO_META;
    using audio::midi::TO_META_LOW;

    if (!m_isPlaying || m_pEvents == nullptr)
        return;

    if (m_paused)
        return;

    if (m_delta_micro >= 1'000'000 / CLOCK_HZ)
    {
        m_delta_micro -= 1'000'000 / CLOCK_HZ;
        return;
    }

    if (m_pEvent != nullptr)
    {
        m_midiDriver->send(*m_pEvent);
        m_pEvent = nullptr;
        ++m_pos;
        return;
    }

    if (m_pos >= m_pEvents->size())
    {
        m_isPlaying = false;
        return;
    }

    const auto& e = (*m_pEvents)[m_pos];
    switch (TO_HIGH(e.type._.high))
    {
    case MIDI_EVENT_TYPES_HIGH::META_SYSEX:
    {
        switch (TO_META_LOW(e.type._.low))
        {
        case MIDI_META_EVENT_TYPES_LOW::META:
        {
            const uint8_t type = e.data[0];    // must be < 128
            std::string   str;
            switch (TO_META(type))
            {
            case MIDI_META_EVENT::CHANNEL_PREFIX:
                logW(std::format("CHANNEL_PREFIX {:d} not implemented", e.data[1]));
                break;
            case MIDI_META_EVENT::COPYRIGHT:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logT(std::format("CopyRight: {}", str));
                break;
            case MIDI_META_EVENT::CUE_POINT:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logD(std::format("Cue Point: {}", str));
                break;
            case MIDI_META_EVENT::DEVICE_NAME:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logW(std::format("[Not Implemented] Device Name: {}", str));
                break;
            case MIDI_META_EVENT::END_OF_TRACK:
                logD("MIDI end of track.");
                break;
            case MIDI_META_EVENT::INSTRUMENT_NAME:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logT(std::format("Instrument name: {}", str));
                break;
            case MIDI_META_EVENT::KEY_SIGNATURE:
                logT(std::format("KEY_SIGNATURE: {:d} {:d}", e.data[1], e.data[2]));
                break;
            case MIDI_META_EVENT::LYRICS:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logT(std::format("Lyrics: {}", str));
                break;
            case MIDI_META_EVENT::MARKER:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logT(std::format("Marker: {}", str));
                break;
            case MIDI_META_EVENT::MIDI_PORT:
                logW(std::format("MIDI_PORT {:d} not implemented", e.data[1]));
                break;
            case MIDI_META_EVENT::PROGRAM_NAME:
                str = utils::chars_vector_to_string_skip_first(e.data);
                logT(std::format("PROGRAM_NAME: {}", str));
                break;
            case MIDI_META_EVENT::SEQUENCER_SPECIFIC:
                logW("SEQUENCE_SPECIFIC not implemented");
                break;
            case MIDI_META_EVENT::SEQUENCE_NAME:    // a.k.a track name
                str = utils::chars_vector_to_string(++(e.data.begin()), e.data.end());
                logT(std::format("SEQUENCE NAME: {}", str));
                break;
            case MIDI_META_EVENT::SEQUENCE_NUMBER:
                logW("Sequence number not implemented");
                break;
            case MIDI_META_EVENT::SET_TEMPO:
            {
                setTempo((e.data[1] << 16) + (e.data[2] << 8) + (e.data[3]));
                const auto tempo_micros = tempo_to_micros(m_tempo, m_division);
                m_delta_step            = tempo_micros;
                logT(std::format("Tempo {}, ({} bpm) -- microseconds/tick {}", m_tempo.load(), 60000000 / m_tempo.load(), tempo_micros));
                break;
            }
            case MIDI_META_EVENT::SMPTE_OFFSET:
                logW("SMPTE_OFFSET not implemented");
                break;
            case MIDI_META_EVENT::TEXT:
                str = utils::chars_vector_to_string(++(e.data.begin()), e.data.end());
                logT(std::format("Text: {}", str));
                break;
            case MIDI_META_EVENT::TIME_SIGNATURE:
                logT(std::format("TIME_SIGNATURE: {:d}/{:d} - clocks {:d} - bb {:d} ", e.data[1], utils::powerOf2(e.data[2]), e.data[3], e.data[4]));
                break;
            default:
                logW(std::format("MIDI_META_EVENT_TYPES_LOW not implemented/recognized: {:#02x}", type));
                break;
            }
            ++m_pos;
            return;    // META event processed, go on next MIDI event
        }
        case MIDI_META_EVENT_TYPES_LOW::SYS_EX0:
            logD("SYS_EX0 META event");
            // TODO: it should be sent as normal event?
            // (it is processed as a normal event now in IMidiDriver)
            m_midiDriver->send(e);
            ++m_pos;
            return;
        case MIDI_META_EVENT_TYPES_LOW::SYS_EX7:
            logD("SYS_EX7 META event");
            // TODO: it should be sent as normal event?
            m_midiDriver->send(e);
            ++m_pos;
            return;
        default:
            logW(std::format("MIDI_META_EVENT_TYPES_LOW not implemented/recognized: {:#02x}", e.type._.low));
            break;
        }
    }
    break;

    case MIDI_EVENT_TYPES_HIGH::NOTE_OFF:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::NOTE_ON:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::AFTERTOUCH:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::CONTROLLER:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::PITCH_BEND:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::PROGRAM_CHANGE:
        [[fallthrough]];
    case MIDI_EVENT_TYPES_HIGH::CHANNEL_AFTERTOUCH:
        break;
    default:
        logW(std::format("unrecognized MIDI EVENT type high {:#02x}", e.type._.high));
        break;
    }

    if (e.delta_time != 0)
    {
        // m_delta_ticks = e.delta_time;
        m_delta_micro = e.delta_time * m_delta_step;
        m_pEvent      = &e;
        return;
    }

    m_midiDriver->send(e);
    ++m_pos;
}

}    // namespace HyperSonicDrivers::drivers
