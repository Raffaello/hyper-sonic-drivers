#pragma once

#include <cstdint>
#include <HyperSonicDrivers/drivers/midi/IMidiChannelVoice.hpp>
#include <HyperSonicDrivers/audio/opl/banks/OP2Bank.hpp>
#include <HyperSonicDrivers/hardware/opl/OPL2instrument.h>
#include <HyperSonicDrivers/drivers/opl/OplWriter.hpp>

namespace HyperSonicDrivers::drivers::midi::opl
{
constexpr int opl_sustain_threshold = 64;

/// <summary>
/// This is only the execution for a MidiChannel
/// used in a MIDI Channel -> Opl Channel
/// </summary>
class OplVoice : public IMidiChannelVoice
{
public:
    explicit OplVoice(const uint8_t slot, const drivers::opl::OplWriter* oplWriter);
    ~OplVoice() override = default;

    /// <summary>
    /// It might release the note depending on sustains value
    /// </summary>
    /// <param name="note"></param>
    /// <param name="sustain"></param>
    /// <returns>true = voice released. false=voice sustained</returns>
    bool noteOff(const uint8_t note, const uint8_t sustain) noexcept;
    bool pitchBend(const uint16_t bend) noexcept;
    bool ctrl_modulationWheel(const uint8_t value) noexcept;
    bool ctrl_volume(const uint8_t value) noexcept;
    bool ctrl_panPosition(const uint8_t value) noexcept;
    bool releaseSustain() noexcept;

    void playNote(const bool keyOn) const noexcept;
    /// <summary>
    /// This works only with OP2Bank.
    /// TODO: need to generalize OplBank instruments ...
    /// </summary>
    int allocate(IMidiChannel*                                 channel,
                 const uint8_t                                 note,
                 const uint8_t                                 volume,
                 const audio::opl::banks::Op2BankInstrument_t* instrument,
                 const bool                                    secondary) noexcept;

    uint8_t release(const bool forced) noexcept;
    void    pause() const noexcept;
    void    resume() const noexcept;

    inline const hardware::opl::OPL2instrument_t* getInstrument() const noexcept { return m_instr; }

    // Methods to get private variables, not really used
    inline uint8_t getSlot() const noexcept { return m_slot; }

    inline bool isSecondary() const noexcept { return m_secondary; }

protected:
    void setInstrument(const hardware::opl::OPL2instrument_t* instr) noexcept;

private:
    const drivers::opl::OplWriter*         m_oplWriter;
    const uint8_t                          m_slot;                /* OPL channel number */
    uint8_t                                m_real_note = 0;       /* adjusted note number */
    int8_t                                 m_finetune  = 0;       /* frequency fine-tune */
    const hardware::opl::OPL2instrument_t* m_instr     = nullptr; /* current instrument */

    bool m_secondary = false;
};
}    // namespace HyperSonicDrivers::drivers::midi::opl
