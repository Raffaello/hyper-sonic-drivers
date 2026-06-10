#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/audio/midi/MIDIEvent.hpp>
#include <HyperSonicDrivers/drivers/midi/IMidiChannel.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/hardware/IHardware.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::drivers::midi
{
/**
 * Interface for all the different Midi drivers
 * so it can be easily used from MIDDrv (maybe rename it to MIDI_player or something)
 * TODO/NOTE: it might be "merged" with IMusicDriver ...
 **/
class IMidiDriver
{
public:
    static constexpr uint32_t DEFAULT_MIDI_TEMPO = 500000;
    static constexpr int32_t  CLOCK_HZ           = 10'000;

    IMidiDriver()          = default;
    virtual ~IMidiDriver() = default;

    virtual bool open(const audio::mixer::eChannelGroup group,
                      const uint8_t                     volume,
                      const uint8_t                     pan) = 0;
    virtual void close()                                     = 0;

    virtual void send(const audio::midi::MIDIEvent& e) noexcept;
    virtual void send(const int8_t channel, const uint32_t msg) noexcept;
    virtual void send(const uint32_t msg) noexcept;
    virtual void send(const audio::midi::MIDI_EVENT_TYPES_HIGH type, const uint8_t channel, const uint8_t data1, const uint8_t data2);

    inline bool     isOpen() const noexcept;
    inline void     pause() noexcept;
    inline void     resume() noexcept;
    inline bool     isPlaying() const noexcept;
    inline bool     isPaused() const noexcept;
    inline void     stop() noexcept;
    inline bool     isTempoChanged() const noexcept;
    inline uint32_t getTempo() noexcept;

    void play(const std::vector<audio::midi::MIDIEvent>& events, uint16_t division);

protected:
    bool     m_isOpen           = false;
    bool     m_isPlaying        = false;
    bool     m_paused           = false;
    bool     m_midiTempoChanged = false;
    uint16_t m_delta_step       = 0;
    uint16_t m_division         = 0;
    uint32_t m_tempo            = 0;
    uint32_t m_pos              = 0;
    uint32_t m_delta_micro      = 0;

    const std::vector<audio::midi::MIDIEvent>* m_pEvents = nullptr;
    const audio::midi::MIDIEvent*              m_pEvent  = nullptr;

    std::array<std::unique_ptr<IMidiChannel>, audio::midi::MIDI_MAX_CHANNELS> m_channels;

    // virtual void onCallback() noexcept = 0; // TODO: consider to re-do it

    // virtual void onOpen() = 0;
    virtual void onPause() noexcept  = 0;
    virtual void onResume() noexcept = 0;

    void callback_();

    inline void setTempo(const uint32_t tempo) noexcept;

    // MIDI events
    virtual void noteOff(const uint8_t chan, const uint8_t note) noexcept                   = 0;
    virtual void noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept = 0;
    virtual void controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept;
    virtual void programChange(const uint8_t chan, const uint8_t program) noexcept;
    virtual void pitchBend(const uint8_t chan, const uint16_t bend) noexcept = 0;

    /**
     * Transmit a SysEx to the MIDI device.
     *
     * The given msg MUST NOT contain the usual SysEx frame, i.e.
     * do NOT include the leading 0xF0 and the trailing 0xF7.
     *
     * Furthermore, the maximal supported length of a SysEx
     * is 264 bytes. Passing longer buffers can lead to
     * undefined behavior (most likely, a crash).
     * TODO: review this method
     */
    virtual void sysEx(const uint8_t* msg, uint16_t length) noexcept = 0;

    // MIDI Controller Events
    virtual void ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept = 0;
    virtual void ctrl_volume(const uint8_t chan, const uint8_t value) noexcept          = 0;
    virtual void ctrl_panPosition(const uint8_t chan, const uint8_t value) noexcept     = 0;
    virtual void ctrl_sustain(const uint8_t chan, const uint8_t value) noexcept         = 0;
    virtual void ctrl_reverb(const uint8_t chan, const uint8_t value) noexcept          = 0;
    virtual void ctrl_chorus(const uint8_t chan, const uint8_t value) noexcept          = 0;
    virtual void ctrl_allNotesOff() noexcept                                            = 0;

    // virtual void pitchBendFactor(uint8_t value) noexcept = 0;
    // virtual void transpose(int8_t value) noexcept = 0;
    // virtual void detune(uint8_t value) noexcept = 0; //{ controlChange(17, value); }
    // virtual void priority(uint8_t value) noexcept = 0; //{ }
};

inline bool IMidiDriver::isOpen() const noexcept
{
    return m_isOpen;
}

inline void IMidiDriver::pause() noexcept
{
    if (m_isPlaying && !m_paused)
    {
        m_paused = true;
        onPause();
    }
};

inline void IMidiDriver::resume() noexcept
{
    if (m_isPlaying && m_paused)
    {
        m_paused = false;
        onResume();
    }
};

inline bool IMidiDriver::isPlaying() const noexcept
{
    return m_isPlaying;
}

inline bool IMidiDriver::isPaused() const noexcept
{
    return isPlaying() && m_paused;
}

inline void IMidiDriver::stop() noexcept
{
    m_paused    = false;
    m_isPlaying = false;
    // TODO do virtual onStop to stop all the sounds
}

inline bool IMidiDriver::isTempoChanged() const noexcept
{
    return m_midiTempoChanged;
}

inline uint32_t IMidiDriver::getTempo() noexcept
{
    m_midiTempoChanged = false;
    return m_tempo;
}

inline void IMidiDriver::setTempo(const uint32_t tempo) noexcept
{
    m_midiTempoChanged = true;
    m_tempo            = tempo;
    if (m_division == 0)
    {
        utils::logW("m_division = 0");
        m_delta_step = 0xFFFF;
    }
    else
        m_delta_step = static_cast<uint16_t>(static_cast<float>(m_tempo) / static_cast<float>(m_division));
}

}    // namespace HyperSonicDrivers::drivers::midi
