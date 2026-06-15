#pragma once

#include <cstdint>
#include <memory>
#include <HyperSonicDrivers/drivers/midi/IMidiDriver.hpp>
#include <HyperSonicDrivers/devices/MT32.hpp>
#include <HyperSonicDrivers/hardware/mt32/MT32.hpp>

namespace HyperSonicDrivers::drivers::midi::mt32
{
class MT32Driver : public IMidiDriver
{
public:
    explicit MT32Driver(const std::shared_ptr<devices::MT32>& mt32);
    ~MT32Driver() override;

    bool open(const audio::mixer::eChannelGroup group,
              const uint8_t                     volume,
              const uint8_t                     pan) override;
    void close() override;

    void send(const uint32_t msg) noexcept override;

protected:
    // void onCallback() noexcept override;
    void onStop() noexcept override {};
    void onPause() noexcept override {};
    void onResume() noexcept override {};

    // MIDI events (not implemented, directly send MIDI msg to MT32Emu service)
    void noteOff(const uint8_t chan, const uint8_t note) noexcept override;
    void noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept override;
    void controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept override;
    void programChange(const uint8_t chan, const uint8_t program) noexcept override;
    void pitchBend(const uint8_t chan, const uint16_t bend) noexcept override;
    void sysEx(const uint8_t* msg, uint16_t length) noexcept override;
    // MIDI Controller Events
    void ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_volume(const uint8_t chan, const uint8_t value) noexcept override;
    void ctrl_panPosition(const uint8_t chan, uint8_t value) noexcept override;
    void ctrl_sustain(const uint8_t chan, uint8_t value) noexcept override;
    void ctrl_reverb(const uint8_t chan, uint8_t value) noexcept override;
    void ctrl_chorus(const uint8_t chan, uint8_t value) noexcept override;
    void ctrl_allNotesOff() noexcept override;

private:
    std::shared_ptr<hardware::mt32::MT32> m_mt32;
};
}    // namespace HyperSonicDrivers::drivers::midi::mt32
