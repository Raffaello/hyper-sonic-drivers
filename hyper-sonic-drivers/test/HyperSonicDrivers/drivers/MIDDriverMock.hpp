#pragma once

#include <HyperSonicDrivers/audio/midi/MIDITrack.hpp>
#include <HyperSonicDrivers/devices/IDevice.hpp>
#include <HyperSonicDrivers/drivers/MIDDriver.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <memory>

namespace HyperSonicDrivers::drivers
{
class MIDDriverMock : public MIDDriver
{
public:
    explicit MIDDriverMock(const std::shared_ptr<devices::IDevice>& device) : MIDDriver(device, audio::mixer::eChannelGroup::Unknown)
    {
    }

    void play(const uint16_t track = 0) noexcept override
    {
        // TODO. not mocking at the moment. need an idea
        MIDDriver::play(track);
        // int i = 0;
        // while (isPlaying() && i < 1000)
        // {
        //     i++;
        //     utils::delayMillis(1);
        // };
    }
};
}    // namespace HyperSonicDrivers::drivers
