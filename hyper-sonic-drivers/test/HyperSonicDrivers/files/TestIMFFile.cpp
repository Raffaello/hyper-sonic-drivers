#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <cstdint>
#include <memory>

// test
// #include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
// #include <HyperSonicDrivers/drivers/opl/OplWriter.hpp>
// #include <HyperSonicDrivers/drivers/midi/opl/OplDriver.hpp>
// #include <HyperSonicDrivers/devices/Adlib.hpp>
// #include <HyperSonicDrivers/utils/algorithms.hpp>
// #include <array>
// #include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::files
{
TEST(IMFFile, cstorDefault)
{
    IMFFile f("../fixtures/01.imf");

    // utils::ILogger::instance->setLevelAll(utils::ILogger::eLevel::Debug);
    // auto data = f.data();
    // EXPECT_EQ(data.size(), 2107);

    // auto mixer = audio::make_mixer<audio::rtaudio::Mixer>(4, 44100, 1024);
    // mixer->init();

    // auto adlib = devices::make_device<devices::Adlib>(mixer);
    // adlib->init();

    // auto opl = dynamic_cast<devices::Adlib*>(adlib.get())->getOpl();

    // drivers::opl::OplWriter opl_writer(opl, false);
    // opl_writer.init();
    // opl->init();

    // std::array<bool, 9> used_channels;
    // used_channels.fill(false);
    // for (const auto& packet : data)
    // {
    //     uint8_t ch = 1;
    //     if (packet.reg >= 0xB0 && packet.reg <= 0xB8)
    //         used_channels[packet.reg & 0x0F] = (packet.val >> 5) & 0x01;

    // opl_writer.writeValue(packet.reg, 0, packet.val);
    // if (packet.reg >= 0xB0 && packet.reg <= 0xB8)
    //     used_channels[packet.reg & 0x0F] = (packet.val >> 5) & 0x01;
    // if (packet.delay_ticks > 0)
    //     utils::delayMillis(packet.delay_ticks * 700 / 1000);    // 700 MHz timing
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
