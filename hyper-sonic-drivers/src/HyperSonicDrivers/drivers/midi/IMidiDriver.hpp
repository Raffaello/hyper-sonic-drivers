#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/audio/midi/MIDIEvent.hpp>
#include <HyperSonicDrivers/drivers/midi/IMidiChannel.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/hardware/IHardware.hpp>

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
    IMidiDriver()          = default;
    virtual ~IMidiDriver() = default;

    virtual bool open(const audio::mixer::eChannelGroup group,
                      const uint8_t                     volume,
                      const uint8_t                     pan) = 0;
    virtual void close()                                     = 0;

    inline bool isOpen() const noexcept { return m_isOpen; }

    virtual void send(const audio::midi::MIDIEvent& e) noexcept;
    virtual void send(const int8_t channel, const uint32_t msg) noexcept;
    virtual void send(const uint32_t msg) noexcept;
    virtual void send(const audio::midi::MIDI_EVENT_TYPES_HIGH type, const uint8_t channel, const uint8_t data1, const uint8_t data2);
    virtual void pause() const noexcept  = 0;
    virtual void resume() const noexcept = 0;

    virtual void setCallback(hardware::TimerCallBack callback, int timerFrequency) = 0;

protected:
    audio::mixer::eChannelGroup m_group;
    uint8_t                     m_volume;
    uint8_t                     m_pan;

    bool                                                                      m_isOpen = false;
    std::array<std::unique_ptr<IMidiChannel>, audio::midi::MIDI_MAX_CHANNELS> m_channels;
    virtual void                                                              onCallback() noexcept = 0;

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
}    // namespace HyperSonicDrivers::drivers::midi
