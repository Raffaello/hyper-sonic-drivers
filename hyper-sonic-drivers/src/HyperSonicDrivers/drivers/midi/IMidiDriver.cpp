#include <HyperSonicDrivers/drivers/midi/IMidiDriver.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <format>

namespace HyperSonicDrivers::drivers::midi
{
using audio::midi::TO_HIGH;
using utils::logW;
using utils::logD;
using utils::logT;

constexpr uint32_t DEFAULT_MIDI_TEMPO = 500000;
constexpr int32_t  CLOCK_HZ           = 10'000;

void IMidiDriver::send(const audio::midi::MIDIEvent& e) noexcept
{
    // TODO: sysEx must be reviewed if ok, probably should check
    //       also if last byte is meta sysEx end ...
    using audio::midi::MIDI_EVENT_TYPES_HIGH;
    using audio::midi::TO_HIGH;

    if (TO_HIGH(e.type._.high) == MIDI_EVENT_TYPES_HIGH::META_SYSEX)
        sysEx(e.data.data(), static_cast<uint16_t>(e.data.size()));
    else
        send(e.toUint32());
}

void IMidiDriver::send(int8_t channel, uint32_t msg) noexcept
{
    using audio::midi::MIDI_EVENT_type_u;
    using audio::midi::MIDI_EVENT_TYPES_HIGH;

    const uint8_t     param2 = static_cast<uint8_t>((msg >> 16) & 0xFF);
    const uint8_t     param1 = static_cast<uint8_t>((msg >> 8) & 0xFF);
    MIDI_EVENT_type_u cmd;
    cmd._.high = static_cast<uint8_t>((msg >> 4) & 0xF);

    send(TO_HIGH(cmd._.high), channel, param1, param2);
}

void IMidiDriver::send(uint32_t msg) noexcept
{
    send(msg & 0xF, msg & 0xFFFFFFF0);
}

void IMidiDriver::send(const audio::midi::MIDI_EVENT_TYPES_HIGH type, const uint8_t channel, const uint8_t data1, const uint8_t data2)
{
    using audio::midi::TO_CTRL;

    switch (type)
    {
        using enum audio::midi::MIDI_EVENT_TYPES_HIGH;

    case NOTE_OFF:    // Note Off
        noteOff(channel, data1);
        break;
    case NOTE_ON:    // Note On
        noteOn(channel, data1, data2);
        break;
    case AFTERTOUCH:    // Aftertouch
        break;          // Not supported.
    case CONTROLLER:    // Control Change
        controller(channel, TO_CTRL(data1), data2);
        break;
    case PROGRAM_CHANGE:    // Program Change
        programChange(channel, data1);
        break;
    case CHANNEL_AFTERTOUCH:    // Channel Pressure
        break;                  // Not supported.
    case PITCH_BEND:            // Pitch Bend
    {
        const auto bend = static_cast<uint16_t>((data1 | (data2 << 7)) - audio::midi::MIDI_PITCH_BEND_DEFAULT);
        pitchBend(channel, bend);
    }
    break;
    case META_SYSEX:    // SysEx
        // We should never get here! SysEx information has to be
        // sent via high-level semantic methods.
        logW("Receiving SysEx command on a send() call");
        break;

    default:
        logW(std::format("Unknown send() command {:#0x}", static_cast<uint8_t>(type)));
    }
}

void IMidiDriver::controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept
{
    // MIDI_EVENT_CONTROLLER_TYPES
    switch (ctrl_type)
    {
        using enum audio::midi::MIDI_EVENT_CONTROLLER_TYPES;
    case BANK_SELECT_MSB:
        logW(std::format("bank select value {}", value));
        break;
    case MODULATION_WHEEL:
        ctrl_modulationWheel(chan, value);
        break;
    case CHANNEL_VOLUME:
        ctrl_volume(chan, value);
        break;
    case PAN:
        ctrl_panPosition(chan, value);
        break;
    case GENERAL_PURPOSE_CONTROLLER_1:
        // pitchBendFactor(value);
        logW(std::format("pitchBendFactor value {}", value));
        break;
    case GENERAL_PURPOSE_CONTROLLER_2:
        // detune(value);
        logW(std::format("detune value {}", value));
        break;
    case GENERAL_PURPOSE_CONTROLLER_3:
        // priority(value);
        logW(std::format("priority value {}", value));
        break;
    case SUSTAIN:
        ctrl_sustain(chan, value);
        break;
    case REVERB:
        ctrl_reverb(chan, value);
        break;
    case CHORUS:
        ctrl_chorus(chan, value);
        break;
    case RESET_ALL_CONTROLLERS:
        // reset all controllers
        logW("reset all controllers value not implemented");
        break;
    case ALL_NOTES_OFF:
        ctrl_allNotesOff();
        break;
    default:
        logW(std::format("OplDriver: Unknown control change message {:d} {:d}", static_cast<uint8_t>(ctrl_type), value));
    }
}

void IMidiDriver::programChange(const uint8_t chan, const uint8_t program) noexcept
{
    if (program > 127)
    {
        logW(std::format("Program change value >= 127 -> {}", program));
    }

    m_channels[chan]->program = program;
}

void IMidiDriver::play(const std::vector<audio::midi::MIDIEvent>& events, uint16_t division)
{
    m_pEvents   = &events;
    m_division  = division & 0x7FFF;
    m_pos       = 0;
    m_isPlaying = true;
    m_paused    = false;

    setTempo(DEFAULT_MIDI_TEMPO);    // 120 BPM;
}

void IMidiDriver::callback_()
{
    using audio::midi::MIDI_EVENT_TYPES_HIGH;
    using audio::midi::MIDI_META_EVENT;
    using audio::midi::MIDI_META_EVENT_TYPES_LOW;
    using audio::midi::MIDI_META_EVENT_VAL;
    using audio::midi::TO_META;
    using audio::midi::TO_META_LOW;

    if (m_pEvents == nullptr)
    {
        m_isPlaying = false;
        return;
    }

    if (!m_isPlaying)
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
        send(*m_pEvent);
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
                m_isPlaying = false;
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
                logT(std::format("Tempo {}, ({} bpm) -- microseconds/tick {}", m_tempo, 60000000 / m_tempo, m_delta_step));
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
            send(e);
            ++m_pos;
            return;
        case MIDI_META_EVENT_TYPES_LOW::SYS_EX7:
            logD("SYS_EX7 META event");
            // TODO: it should be sent as normal event?
            send(e);
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
        m_delta_micro = e.delta_time * m_delta_step;
        m_pEvent      = &e;
        return;
    }

    send(e);
    ++m_pos;
}

}    // namespace HyperSonicDrivers::drivers::midi
