#include <gtest/gtest.h>
#include <HyperSonicDrivers/audio/Renderer.hpp>
#include <HyperSonicDrivers/devices/Adlib.hpp>
#include <HyperSonicDrivers/devices/SbPro2.hpp>
#include <HyperSonicDrivers/drivers/westwood/ADLDriver.hpp>
#include <HyperSonicDrivers/audio/mixer/ChannelGroup.hpp>
#include <HyperSonicDrivers/hardware/opl/OplEmulator.hpp>
#include <HyperSonicDrivers/audio/IMixerMock.hpp>
#include <filesystem>
#include <string>

namespace HyperSonicDrivers::audio
{
using audio::mixer::eChannelGroup;
using hardware::opl::OplEmulator;
using devices::eDeviceName;

class RendererTest : public ::testing::TestWithParam<std::tuple<std::string, const int, const devices::eDeviceName, const OplEmulator>>
{
public:
    const std::string test_name   = std::get<0>(GetParam());
    const int         freq        = std::get<1>(GetParam());
    const eDeviceName device_name = std::get<2>(GetParam());
    const OplEmulator opl_emu     = std::get<3>(GetParam());

    std::shared_ptr<devices::Opl> opl;
    std::shared_ptr<IMixer>       mixer;

    RendererTest()
    {
        mixer = std::make_shared<IMixerMock>();
        switch (device_name)
        {
            using enum devices::eDeviceName;
        case Adlib:
            opl = devices::make_device<devices::Adlib, devices::Opl>(mixer, opl_emu);
            break;
        case SbPro2:
            opl = devices::make_device<devices::SbPro2, devices::Opl>(mixer, opl_emu);
            break;
        default:
            throw std::invalid_argument("");
        }
    }
};

TEST_P(RendererTest, render_wav)
{
    const std::string exp_renderer = "../fixtures/test_renderer_" + test_name + ".wav";
    const std::string rfile        = "../fixtures/test_renderer_" + test_name + "_out.wav";

    if (std::filesystem::exists(rfile))
        std::filesystem::remove(rfile);

    ASSERT_FALSE(std::filesystem::exists(rfile));
    {
        Renderer r(freq, 1024);
        r.openOutputFile(rfile);

        auto drv1 = drivers::westwood::ADLDriver(opl, eChannelGroup::Music);
        auto af   = std::make_shared<files::westwood::ADLFile>("../fixtures/DUNE0.ADL");
        drv1.setADLFile(af);

        drv1.play(4);
        while (drv1.isPlaying())
            r.renderBuffer(opl);

        ASSERT_TRUE(r.renderFlush(opl));
        r.closeOutputFile();
    }

    files::WAVFile w(rfile);
    auto           sound = w.getSound();
    files::WAVFile wexp(exp_renderer);
    auto           exp_sound = wexp.getSound();

    ASSERT_EQ(sound->dataSize, exp_sound->dataSize);
    ASSERT_EQ(sound->freq, exp_sound->freq);
    ASSERT_EQ(sound->stereo, exp_sound->stereo);
    EXPECT_EQ(sound->freq, freq);
    EXPECT_EQ(sound->stereo, opl->getHardware()->isStereo());
    for (uint32_t i = 0; i < sound->dataSize; i++)
    {
        EXPECT_EQ(sound->data[i], exp_sound->data[i]);
    }
}

TEST_P(RendererTest, render_wav2)
{
    const std::string exp_renderer = "../fixtures/test_renderer_" + test_name + ".wav";
    const std::string rfile        = "../fixtures/test_renderer_" + test_name + "_out2.wav";

    if (std::filesystem::exists(rfile))
        std::filesystem::remove(rfile);

    ASSERT_FALSE(std::filesystem::exists(rfile));
    {
        Renderer r(freq, 1024);
        r.openOutputFile(rfile);

        auto drv1 = drivers::westwood::ADLDriver(opl, eChannelGroup::Music);
        auto af   = std::make_shared<files::westwood::ADLFile>("../fixtures/DUNE0.ADL");
        drv1.setADLFile(af);

        ASSERT_TRUE(r.renderBufferFlush(opl, drv1, 4));
        r.closeOutputFile();
    }

    files::WAVFile w(rfile);
    auto           sound = w.getSound();
    files::WAVFile wexp(exp_renderer);
    auto           exp_sound = wexp.getSound();

    ASSERT_EQ(sound->dataSize, exp_sound->dataSize);
    ASSERT_EQ(sound->freq, exp_sound->freq);
    ASSERT_EQ(sound->stereo, exp_sound->stereo);
    EXPECT_EQ(sound->freq, freq);
    EXPECT_EQ(sound->stereo, opl->getHardware()->isStereo());
    for (uint32_t i = 0; i < sound->dataSize; i++)
    {
        EXPECT_EQ(sound->data[i], exp_sound->data[i]);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Renderer,
    RendererTest,
    ::testing::Values(
        std::make_tuple<>("adlib_mame2", 44100, eDeviceName::Adlib, OplEmulator::MAME),
        std::make_tuple<>("sbpro2_dosbox", 44100, eDeviceName::SbPro2, OplEmulator::DOS_BOX)));
}    // namespace HyperSonicDrivers::audio

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
