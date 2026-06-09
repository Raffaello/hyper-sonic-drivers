#pragma once

#include <memory>
#include <cstdint>
#include <HyperSonicDrivers/audio/IMixer.hpp>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/audio/MIDI.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/devices/IDevice.hpp>
#include <HyperSonicDrivers/drivers/IAudioDriver.hpp>
#include <HyperSonicDrivers/audio/opl/banks/OP2Bank.hpp>

namespace HyperSonicDrivers::drivers
{
/**
 * This class is a wrapper around different midi drivers
 **/
class MIDDriver : public IAudioDriver
{
public:
    explicit MIDDriver(
        const std::shared_ptr<devices::IDevice>& device,
        const audio::mixer::eChannelGroup        group,
        const uint8_t                            volume = 255,
        const uint8_t                            pan    = 0);
    ~MIDDriver() override;

    void setMidi(const std::shared_ptr<audio::MIDI>& midi) noexcept;

    /**
     * @brief It works only for OPL devices. This will replace the internal IMidiDriver with a specific OplDriver
     *
     * @param op2Bank
     * @return true
     * @return false
     */
    bool loadBankOP2(const std::shared_ptr<audio::opl::banks::OP2Bank>& op2Bank) noexcept;

    /**
     * @brief this restore the default MidiDriver (scummvm::MidiAdlib, MT32)
     *
     * @return true
     * @return false
     */
    bool resetBankOP2() noexcept;

    void play(const uint16_t track = 0) noexcept override;
    void stop() noexcept override;

    void pause() noexcept;
    void resume() noexcept;

    bool isPlaying() const noexcept override;

    bool isPaused() const noexcept;

    inline bool isTempoChanged() const noexcept { return m_midiDriver != nullptr ? m_midiDriver->isTempoChanged() : false; }

    inline uint32_t getTempo() const noexcept
    {
        return m_midiDriver != nullptr ? m_midiDriver->getTempo() : 0;
    }

protected:
    bool open_() noexcept;

private:
    // this is to abstract the specific midi driver implementation
    std::unique_ptr<drivers::midi::IMidiDriver> m_midiDriver;
    std::shared_ptr<audio::MIDI>                m_midi;

    const audio::mixer::eChannelGroup m_group;
    const uint8_t                     m_volume;
    const uint8_t                     m_pan;
};
}    // namespace HyperSonicDrivers::drivers
