#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <cstdint>
#include <memory>

// test
// #include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
// #include <HyperSonicDrivers/hardware/opl/OPLFactory.hpp>
// #include <HyperSonicDrivers/utils/algorithms.hpp>
// #include <HyperSonicDrivers/utils/ILogger.hpp>
// #if HAS_SDL3
// #include <HyperSonicDrivers/audio/sdl3/Mixer.hpp>
// #include <SDL3/SDL_main.h>
// #define MIXER audio::sdl3::Mixer
// #elif HAS_SDL2
// #include <HyperSonicDrivers/audio/sdl2/Mixer.hpp>
// #include <SDL2/SDL_main.h>
// #define MIXER audio::sdl2::Mixer
// #else
// #include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
// #define MIXER audio::rtaudio::Mixer
// #endif

namespace HyperSonicDrivers::files
{

void opl_callback()
{
    static int delta_ticks = 0;
    delta_ticks++;
}

TEST(IMFFile, cstorDefault)
{
    IMFFile f("../fixtures/02.imf");

    // utils::ILogger::instance->setLevelAll(utils::ILogger::eLevel::Debug);
    auto data = f.data();
    EXPECT_EQ(data.size(), 2084);

    // TEST
    // auto mixer = audio::make_mixer<MIXER>(4, 44100, 1024);
    // ASSERT_TRUE(mixer->init());

    // auto opl = hardware::opl::OPLFactory::create(hardware::opl::OplEmulator::AUTO, hardware::opl::OplType::OPL2, mixer);
    // ASSERT_NE(opl, nullptr);

    // ASSERT_TRUE(opl->init());
    // opl->start(std::make_shared<hardware::TimerCallBack>(opl_callback), audio::mixer::eChannelGroup::Music, 255, 0, opl->setCallbackFrequency(700));

    // for (const auto& packet : data)
    // {
    //     opl->writeReg(packet.reg, packet.val);
    //     if (packet.delay_ticks > 0)
    //         utils::delayMicro(packet.delay_ticks * 700 * 2);    // 700 Hz timing
    // }
}

TEST(IMFFile, file_not_found)
{
    EXPECT_THROW(IMFFile f(""), std::system_error);
}


}    // namespace HyperSonicDrivers::files

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
