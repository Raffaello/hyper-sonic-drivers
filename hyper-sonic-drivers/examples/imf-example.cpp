#include <HyperSonicDrivers/hardware/opl/OPL.hpp>
#include <HyperSonicDrivers/hardware/opl/OPLFactory.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <HyperSonicDrivers/devices/Adlib.hpp>
#include <HyperSonicDrivers/devices/SbPro.hpp>
#include <HyperSonicDrivers/devices/SbPro2.hpp>
#include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <HyperSonicDrivers/drivers/opl/IMFDriver.hpp>

#include <spdlog/spdlog.h>
#include <fmt/color.h>

#include <memory>
#include <cstdint>
#include <map>
#include <string>

#if HAS_SDL3
#include <SDL3/SDL_main.h>
#elif HAS_SDL2
#include <SDL2/SDL_main.h>
#endif

#if defined(FMT_VERSION) && FMT_VERSION > 90000
#define FMT_RUNTIME(x) fmt::runtime(x)
#else
#define FMT_RUNTIME(x) x
#endif

using namespace HyperSonicDrivers;

using files::IMFFile;
using hardware::opl::OplEmulator;
using hardware::opl::OPLFactory;
using hardware::opl::OplType;
using utils::delayMillis;

void imf_test(const OplEmulator emu, const OplType type, std::shared_ptr<audio::IMixer> mixer, const std::string& filename)
{
    using devices::make_device;

    auto                          imfFile = std::make_shared<IMFFile>(filename);
    std::shared_ptr<devices::Opl> device;
    switch (type)
    {
        using enum OplType;

    case OPL2:
        device = make_device<devices::Adlib, devices::Opl>(mixer, emu);
        break;
    case DUAL_OPL2:
        device = make_device<devices::SbPro, devices::Opl>(mixer, emu);
        break;
    case OPL3:
        device = make_device<devices::SbPro2, devices::Opl>(mixer, emu);
        break;
    }

    drivers::opl::IMFDriver imf_driver(device, audio::mixer::eChannelGroup::Music);
    imf_driver.setIMFFile(imfFile);

    imf_driver.play(0);

    // do
    // {
    //     // spdlog::info("is playing");
    //     delayMillis(1000);
    // }
    // while (adlDrv.isPlaying());

    device->shutdown();
}

int main(int argc, char* argv[])
{
    auto mixer = audio::make_mixer<audio::rtaudio::Mixer>(8, 44100, 1024);
    if (!mixer->init())
    {
        spdlog::error("can't init mixer");
        return 1;
    }

    const std::map<OplEmulator, std::string> emus = {
        {OplEmulator::DOS_BOX, "DOS_BOX"},
        {OplEmulator::MAME,    "MAME"   },
        {OplEmulator::NUKED,   "NUKED"  },
        {OplEmulator::WOODY,   "WOODY"  },
    };

    const std::map<OplType, std::string> types = {
        {OplType::OPL2,      "OPL2"     },
        {OplType::DUAL_OPL2, "DUAL_OPL2"},
        {OplType::OPL3,      "OPL3"     },
    };

    const std::string m = "##### {} {} #####";

    spdlog::set_level(spdlog::level::info);
    HyperSonicDrivers::utils::ILogger::instance->setLevelAll(HyperSonicDrivers::utils::ILogger::eLevel::Info);
    for (const auto& emu : emus)
    {
        for (const auto& type : types)
        {
            using enum fmt::color;

            for (const auto& c : {white_smoke, yellow, aqua, lime_green, blue_violet, indian_red})
            {
                spdlog::info(fmt::format(fg(c), FMT_RUNTIME(m), emu.second, type.second));
            }

            try
            {
                imf_test(emu.first, type.first, mixer, "02.imf");
            }
            catch (const std::exception& e)
            {
                spdlog::default_logger()->error(e.what());
            }
        }
    }

    return 0;
}
