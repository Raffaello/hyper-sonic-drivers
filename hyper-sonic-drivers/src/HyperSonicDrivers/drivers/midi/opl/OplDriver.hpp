#pragma once

#include <memory>
#include <cstdint>
#include <list>

#include <HyperSonicDrivers/audio/midi/MIDIEvent.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/audio/opl/banks/OP2Bank.hpp>
#include <HyperSonicDrivers/devices/Opl.hpp>
#include <HyperSonicDrivers/drivers/midi/opl/OplVoice.hpp>
#include <HyperSonicDrivers/drivers/opl/OplWriter.hpp>
#include <HyperSonicDrivers/hardware/opl/OPL2instrument.h>
#include <HyperSonicDrivers/drivers/midi/IMidiDriver.hpp>

namespace HyperSonicDrivers::drivers::midi::opl
{
/// <summary>
/// OPL MidiDriver.
/// Support OPL2 and OPL3 modes
/// Mostly used to Play MUS File at the moment due to Op2Bank
/// TODO: generalize the OP2Bank in OplBank (interface)
/// TODO: support XMI extension.
/// TODO: XMI Bank sounds
/// </summary>
class OplDriver : public IMidiDriver
{
public:
    explicit OplDriver(const std::shared_ptr<devices::Opl>& opl);
    ~OplDriver() override;

    bool open(const audio::mixer::eChannelGroup group,
              const uint8_t                     volume,
              const uint8_t                     pan) override;
    void close() override;

    inline void setOP2Bank(const std::shared_ptr<audio::opl::banks::OP2Bank>& op2Bank) noexcept { m_op2Bank = op2Bank; };

    void pause() const noexcept override;
    void resume() const noexcept override;

    void setCallback(hardware::TimerCallBack callback, int timerFrequency) override;

    inline std::shared_ptr<hardware::opl::OPL> getOpl() const noexcept { return m_opl; };

protected:
    void onCallback() noexcept override;

    // MIDI Events
    void noteOff(const uint8_t chan, const uint8_t note) noexcept override;
    void noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept override;
    void pitchBend(const uint8_t chan, const uint16_t bend) noexcept override;
    void sysEx(const uint8_t* msg, uint16_t length) noexcept override;

    // MIDI Controller Events
    void ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_volume(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_panPosition(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_sustain(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_reverb(const uint8_t chan, const uint8_t value) noexcept override { /*NOT SUPPORTED*/ };
    void ctrl_chorus(const uint8_t chan, const uint8_t value) noexcept override { /*NOT SUPPORTED*/ };
    void ctrl_allNotesOff() noexcept override;

private:
    // TODO: add IDevice too into the parent class as there is a IDevice::init method and shutdown to use
    std::shared_ptr<hardware::opl::OPL>         m_opl;
    std::shared_ptr<audio::opl::banks::OP2Bank> m_op2Bank;

    const bool    m_opl3_mode;
    const uint8_t m_oplNumChannels;

    std::vector<std::unique_ptr<OplVoice>>   m_voices;
    std::unique_ptr<drivers::opl::OplWriter> m_oplWriter;

    // TODO review to make this index more efficient (and its complementary)
    // TODO store this in a list in IMidiChannel instead, so the voices to a channel are already all there, no need of an index
    std::list<uint8_t> m_voicesInUseIndex;
    std::list<uint8_t> m_voicesFreeIndex;

    void    releaseSustain(const uint8_t channel) const noexcept;
    uint8_t releaseVoice(const uint8_t slot, const bool forced);
    int     allocateVoice(const uint8_t slot, const uint8_t channel, const uint8_t note, const uint8_t volume, const audio::opl::banks::Op2BankInstrument_t* instrument, const bool secondary);

    /// <summary>
    /// Get a Free OplVoice slot index if available.
    /// Otherwise it will release the first secondary voice found.
    /// Otherwise, if forced, will release the oldest voice.
    /// </summary>
    /// <param name="abs_time"></param>
    /// <param name="force">release the oldest channel if anything else is available</param>
    /// <returns></returns>
    int8_t getFreeOplVoiceIndex(const bool force);
};
}    // namespace HyperSonicDrivers::drivers::midi::opl
