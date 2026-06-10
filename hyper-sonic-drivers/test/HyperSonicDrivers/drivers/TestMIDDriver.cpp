#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <HyperSonicDrivers/audio/midi/MIDIEvent.hpp>
#include <HyperSonicDrivers/audio/midi/MIDITrack.hpp>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/drivers/MIDDriver.hpp>
#include <HyperSonicDrivers/devices/DeviceMock.hpp>
#include <HyperSonicDrivers/devices/Adlib.hpp>
#include <HyperSonicDrivers/drivers/MIDDriverMock.hpp>
#include <HyperSonicDrivers/files/MIDFile.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::drivers
{
using audio::midi::MIDI_EVENT_TYPES_HIGH;
using audio::midi::MIDI_FORMAT;
using audio::midi::MIDI_META_EVENT;
using audio::midi::MIDI_META_EVENT_TYPES_LOW;
using audio::midi::MIDIEvent;
using utils::ILogger;

TEST(MIDDriver, DISABLED_SEQUENCE_NAME_META_EVENT)
{
    auto device = std::make_shared<devices::DeviceMock<devices::Adlib>>();

    ILogger::instance->setLevel(ILogger::eLevel::Trace, ILogger::eCategory::Audio);

    // ---
    MIDIEvent e;
    e.delta_time  = 0;
    e.type._.high = (uint8_t) MIDI_EVENT_TYPES_HIGH::META_SYSEX;
    e.type._.low  = (uint8_t) MIDI_META_EVENT_TYPES_LOW::META;
    // this can be a parameter
    e.data.push_back((uint8_t) MIDI_META_EVENT::SEQUENCE_NAME);
    std::string s = "sequence_name";
    e.data.insert(e.data.end(), s.begin(), s.end());
    auto midi_track = audio::midi::MIDITrack();
    midi_track.addEvent(e);
    auto m = std::make_shared<audio::MIDI>(audio::midi::MIDI_FORMAT::SINGLE_TRACK, 1, 100);
    m->addTrack(midi_track);

    //::testing::internal::CaptureStdout();
    ::testing::internal::CaptureStderr();
    MIDDriverMock middrv(device);
    middrv.setMidi(m);
    middrv.play(0);
    // auto output = ::testing::internal::GetCapturedStdout();
    auto output2 = ::testing::internal::GetCapturedStderr();

    EXPECT_TRUE(output2.find("SEQUENCE NAME: " + s) != std::string::npos);
}

TEST(MIDDriver, force_stop_on_long_delta_time_delay)
{
    auto device = std::make_shared<devices::DeviceMock<devices::Adlib>>();
    {

        MIDIEvent e;
        e.delta_time  = 0;
        e.type._.high = (uint8_t) MIDI_EVENT_TYPES_HIGH::META_SYSEX;
        e.type._.low  = (uint8_t) MIDI_META_EVENT_TYPES_LOW::META;

        // this can be a parameter
        e.data.push_back((uint8_t) MIDI_META_EVENT::SEQUENCE_NAME);

        std::string s = "sequence_name";
        e.data.insert(e.data.end(), s.begin(), s.end());

        auto midi_track = audio::midi::MIDITrack();
        midi_track.addEvent(e);

        e.delta_time  = 1000;
        e.type._.high = (uint8_t) MIDI_EVENT_TYPES_HIGH::PROGRAM_CHANGE;
        e.type._.low  = 0;
        e.data.clear();
        e.data.push_back((uint8_t) 0);
        e.data.push_back((uint8_t) 0);

        midi_track.addEvent(e);

        auto midi = std::make_shared<audio::MIDI>(MIDI_FORMAT::SINGLE_TRACK, 1, 192);
        midi->addTrack(midi_track);

        MIDDriverMock middrv(device);
        middrv.setMidi(midi);
        middrv.play(0);
        ASSERT_TRUE(middrv.isPlaying());
        auto start = utils::getMillis<uint32_t>();
        utils::delayMillis(20);
        middrv.stop();
        EXPECT_FALSE(middrv.isPlaying());
        auto stop = utils::getMillis<uint32_t>();
        EXPECT_LE(stop - start, 1 * 1000);
        EXPECT_FALSE(middrv.isPlaying());
        EXPECT_TRUE(device->isAcquired());
    }
    EXPECT_FALSE(device->isAcquired());
}

TEST(MIDDriver, getTempo)
{
    auto          mf     = files::MIDFile("../fixtures/midifile_sample.mid");
    auto          device = std::make_shared<devices::DeviceMock<devices::Adlib>>();
    MIDDriverMock md(device);
    EXPECT_EQ(md.getTempo(), 0);
    EXPECT_FALSE(md.isTempoChanged());
    md.setMidi(mf.getMIDI());
    md.play(0);
    while (!md.isTempoChanged())
    {
        utils::delayMillis(10);
    }
    EXPECT_TRUE(md.isTempoChanged());
    EXPECT_EQ(md.getTempo(), 500000);
    EXPECT_FALSE(md.isTempoChanged());
    md.stop();
}
}    // namespace HyperSonicDrivers::drivers

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
