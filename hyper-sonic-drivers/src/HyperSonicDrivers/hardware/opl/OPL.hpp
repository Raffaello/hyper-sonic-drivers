#pragma once

#include <cstdint>
#include <memory>
#include <HyperSonicDrivers/hardware/IHardware.hpp>
#include <HyperSonicDrivers/hardware/opl/OplType.hpp>
#include <HyperSonicDrivers/audio/IMixer.hpp>

namespace HyperSonicDrivers::hardware::opl
{
constexpr uint8_t opl2_num_channels         = 9;
constexpr uint8_t opl3_num_channels         = 18;
constexpr int     default_opl_callback_freq = 250;

/**
 * A representation of a Yamaha OPL chip.
 */
class OPL : public IHardware
{
public:
    explicit OPL(const std::shared_ptr<audio::IMixer>& mixer, const OplType type);
    ~OPL() override = default;

    const OplType type;

    inline bool isStereo() const noexcept override { return type != OplType::OPL2; }

    /**
     * Writes a byte to the given I/O port.
     * @param port
     * @param val value, which will be written (8 bits, 16 due to OPL3 2nd register)
     */
    virtual void write(const uint32_t port, const uint16_t val) noexcept = 0;

    /**
     * Reads a byte from the given I/O port.
     * @return value read
     */
    virtual uint8_t read(const uint32_t port) noexcept = 0;

    /**
     * TODO review this logic of 0x100 2nd register
     *      as it looks ok, but not well implemented for the interface
     *      especially if it is an OPL2
     *
     * Function to directly write to a specific OPL register.
     * This writes to *both* chips for a Dual OPL2. We allow
     * writing to secondary OPL registers by using register
     * values >= 0x100.
     *
     * @param r hardware register number to write to (8 bits, 16 bits for 2nd register in OPL3)
     * @param v value, which will be written (8 bits, 16 bits for consistency with write method)
     */
    virtual void writeReg(const uint16_t r, const uint16_t v) noexcept = 0;

    /**
     * Start the OPL with callbacks.
     */
    void start(
        const std::shared_ptr<TimerCallBack>& callback,
        const audio::mixer::eChannelGroup     group          = audio::mixer::eChannelGroup::Plain,
        const uint8_t                         volume         = 255,
        const uint8_t                         pan            = 0,
        const int                             timerFrequency = default_opl_callback_freq) override;

protected:
    void startCallback(
        const audio::mixer::eChannelGroup group,
        const uint8_t                     volume,
        const uint8_t                     pan,
        const int                         timerFrequency) override;
};
}    // namespace HyperSonicDrivers::hardware::opl
