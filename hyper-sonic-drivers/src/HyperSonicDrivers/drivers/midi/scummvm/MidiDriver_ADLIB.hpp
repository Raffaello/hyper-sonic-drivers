#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <HyperSonicDrivers/devices/Opl.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/MidiDriver.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/AdLibInstrument.h>
#include <HyperSonicDrivers/drivers/midi/scummvm/AdlibVoice.h>
#include <HyperSonicDrivers/drivers/midi/scummvm/AdLibChannel.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/AdLibPercussionChannel.hpp>

namespace HyperSonicDrivers::drivers::midi::scummvm
{
struct AdLibSetParams
{
    uint8_t registerBase;
    uint8_t shift;
    uint8_t mask;
    uint8_t inversion;
};

class MidiDriver_ADLIB : public MidiDriver
{
public:
    explicit MidiDriver_ADLIB(const std::shared_ptr<devices::Opl>& opl);
    ~MidiDriver_ADLIB() override;

    bool open(const audio::mixer::eChannelGroup group,
              const uint8_t                     volume,
              const uint8_t                     pan) override;
    void close() override;

    void pause() const noexcept override { /*TODO*/ };
    void resume() const noexcept override { /*TODO*/ };

    void setCallback(const hardware::TimerCallBack& callback, int timerFrequency) override;

    uint32_t property(int prop, uint32_t param) override;
    // uint32_t getBaseTempo() override { return 1000000 / hardware::opl::default_opl_callback_freq; }

    void setPitchBendRange(uint8_t channel, unsigned int range) override;
    void sysEx_customInstrument(uint8_t channel, uint32_t type, const uint8_t* instr) override;

    // virtual void setTimerCallback(void* timerParam, /*Common::TimerManager::TimerProc*/ void* timerProc);

protected:
    void onCallback() noexcept override;

    // MIDI Events
    void noteOff(const uint8_t chan, const uint8_t note) noexcept override;
    void noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept override;
    void controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept override;
    void programChange(const uint8_t chan, const uint8_t program) noexcept override;
    void pitchBend(const uint8_t chan, const uint16_t bend) noexcept override;
    void sysEx(const uint8_t* msg, uint16_t length) noexcept override;

    // MIDI Controller Events
    void ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_volume(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_panPosition(const uint8_t chan, const uint8_t value) noexcept override;
    // SCUMM GM Midi driver ctrl exclusive?
    void ctrl_pitchBendFactor(const uint8_t chan, const uint8_t value) noexcept;
    void ctrl_detune(const uint8_t chan, const uint8_t value) noexcept;
    void ctrl_priority(const uint8_t chan, const uint8_t value) noexcept;
    // ---
    void ctrl_sustain(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_reverb(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_chorus(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_allNotesOff() noexcept override;

private:
    std::shared_ptr<hardware::opl::OPL> m_opl;
    bool                                _scummSmallHeader = false;    // FIXME: This flag controls a special mode for SCUMM V3 games
    bool                                m_opl3Mode;

    uint8_t* _regCache          = nullptr;
    uint8_t* _regCacheSecondary = nullptr;

    // Common::TimerManager::TimerProc _adlibTimerProc;
    // void*                           _adlibTimerParam = nullptr;

    int _timerCounter = 0;

    std::array<uint16_t, 9>   _channelTable2;
    std::array<uint16_t, 9>   _curNotTable;
    std::array<AdLibVoice, 9> m_voices;
    AdLibPercussionChannel*   m_percussion;
    int                       _voiceIndex     = -1;
    int                       _timerIncrease  = 0xD69;
    int                       _timerThreshold = 0x411B;

    AdLibChannel* getChannel(const uint8_t channel) const noexcept;

    void partKeyOn(AdLibChannel* part, const AdLibInstrument* instr, uint8_t note, uint8_t velocity, const AdLibInstrument* second, uint8_t pan);
    void partKeyOff(AdLibChannel* part, uint8_t note);

    void    adlibKeyOff(int chan);
    void    adlibNoteOn(int chan, uint8_t note, int mod);
    void    adlibNoteOnEx(int chan, uint8_t note, int mod);
    int     adlibGetRegValueParam(int chan, uint8_t data);
    void    adlibSetupChannel(int chan, const AdLibInstrument* instr, uint8_t vol1, uint8_t vol2);
    void    adlibSetupChannelSecondary(int chan, const AdLibInstrument* instr, uint8_t vol1, uint8_t vol2, uint8_t pan);
    uint8_t adlibGetRegValue(uint8_t reg) const noexcept;
    uint8_t adlibGetRegValueSecondary(uint8_t reg) const noexcept;
    void    adlibSetParam(int channel, uint8_t param, int value, bool primary = true);
    void    adlibKeyOnOff(int channel);
    void    adlibWrite(uint8_t reg, uint8_t value);
    void    adlibWriteSecondary(uint8_t reg, uint8_t value);
    void    adlibPlayNote(int channel, int note);

    AdLibVoice* allocateVoice(uint8_t pri);

    void mcOff(AdLibVoice* voice);

    static void linkMc(AdLibChannel* part, AdLibVoice* voice);
    void        mcIncStuff(AdLibVoice* voice, Struct10* s10, Struct11* s11);
    void        mcInitStuff(AdLibVoice* voice, Struct10* s10, Struct11* s11, uint8_t flags, const InstrumentExtra* ie);

    void           struct10Init(Struct10* s10, const InstrumentExtra* ie);
    static uint8_t struct10OnTimer(Struct10* s10, Struct11* s11);
    static void    struct10Setup(Struct10* s10);
    static int     randomNr(int a);
    void           mcKeyOn(AdLibVoice* voice, const AdLibInstrument* instr, uint8_t note, uint8_t velocity, const AdLibInstrument* second, uint8_t pan);
};
}    // namespace HyperSonicDrivers::drivers::midi::scummvm
