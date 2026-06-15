#pragma once

#include <HyperSonicDrivers/drivers/MIDDriver.hpp>
#include <HyperSonicDrivers/devices/Adlib.hpp>
#include <HyperSonicDrivers/devices/SbPro.hpp>
#include <HyperSonicDrivers/devices/SbPro2.hpp>
#include <HyperSonicDrivers/devices/IDevice.hpp>
#include <HyperSonicDrivers/files/dmx/OP2File.hpp>

#include <HyperSonicDrivers/hardware/opl/OPLFactory.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <std/OplTypeFormatter.hpp>

#include <memory>
#include <spdlog/spdlog.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <map>

#if HAS_SDL3
#include <HyperSonicDrivers/audio/sdl3/Mixer.hpp>
#include <SDL3/SDL_main.h>
#define MIXER audio::sdl3::Mixer
#elif HAS_SDL2
#include <HyperSonicDrivers/audio/sdl2/Mixer.hpp>
#include <SDL2/SDL_main.h>
#define MIXER audio::sdl2::Mixer
#else
#include <HyperSonicDrivers/audio/rtaudio/Mixer.hpp>
#define MIXER audio::rtaudio::Mixer
#endif

#if defined(FMT_VERSION) && FMT_VERSION > 90000
#define FMT_RUNTIME(x) fmt::runtime(x)
#else
#define FMT_RUNTIME(x) x
#endif

using namespace HyperSonicDrivers;

using audio::mixer::eChannelGroup;
using hardware::opl::OplEmulator;
using hardware::opl::OPLFactory;
using hardware::opl::OplType;

void mid_test_run(drivers::MIDDriver& midDrv, const std::shared_ptr<audio::MIDI>& midi)
{
    auto start_time = std::chrono::system_clock::now();
    midDrv.setMidi(midi);
    midDrv.play(0);
    int i = 0;
    while (midDrv.isPlaying())
    {
        utils::delayMillis(1000);
        i++;
        if (i == 10)
        {
            spdlog::info("Pause 3s...");
            midDrv.pause();
        }
        else if (i == 10 + 3)
        {
            spdlog::info("resuming");
            midDrv.resume();
        }
        else if (i == 20)
        {
            spdlog::info("stop (3s)");
            midDrv.stop();
            utils::delayMillis(3000);
            spdlog::info("play again");
            midDrv.play();
        }
        else if (i == 25)
            midDrv.stop();
    }

    auto end_time = std::chrono::system_clock::now();
    auto tot_time = end_time - start_time;
    spdlog::info("Total Running Time: {:%M:%S}", tot_time);
}

void scummvm_mid_test(const OplEmulator emu, const OplType type, const std::shared_ptr<audio::IMixer>& mixer, const std::shared_ptr<audio::MIDI> midi)
{
    std::shared_ptr<devices::IDevice> device;
    switch (type)
    {
        using enum OplType;
        using namespace devices;

    case OPL2:
        device = make_device<devices::Adlib>(mixer, emu);
        break;
    case DUAL_OPL2:
        device = make_device<devices::SbPro>(mixer, emu);
        break;
    case OPL3:
        device = make_device<devices::SbPro2>(mixer, emu);
        break;
    default:
        throw std::runtime_error("?");
    }

    drivers::MIDDriver midDrv(device, eChannelGroup::Music);
    spdlog::info(std::format("playing midi (OPL type={})...", type));
    mid_test_run(midDrv, midi);
}

void mid_test(const OplEmulator emu, const OplType type, const std::shared_ptr<audio::IMixer>& mixer, const std::shared_ptr<audio::MIDI> midi)
{
    auto                              op2file = files::dmx::OP2File("GENMIDI.OP2");
    std::shared_ptr<devices::IDevice> device;
    switch (type)
    {
        using enum OplType;
        using namespace devices;

    case OPL2:
        device = make_device<Adlib>(mixer, emu);
        break;
    case DUAL_OPL2:
        device = make_device<SbPro>(mixer, emu);
        break;
    case OPL3:
        device = make_device<SbPro2>(mixer, emu);
        break;
    default:
        throw std::runtime_error("?");
    }

    drivers::MIDDriver midDrv(/*mixer,*/ device, eChannelGroup::Music);
    if (!midDrv.loadBankOP2(op2file.getBank()))
    {
        spdlog::error("can't load BankOP2");
        return;
    }

    spdlog::info(std::format("playing midi (OPL type={})...", type));
    mid_test_run(midDrv, midi);
}

int run(const std::shared_ptr<audio::MIDI>& midi, const bool use_opldrv)
{
    auto mixer = audio::make_mixer<MIXER>(8, 44100, 1024);
    if (!mixer->init())
    {
        spdlog::error("can't init the mixer");
        return 1;
    }

    // Reproducing MIDI file
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

    // Emulators
    using enum fmt::color;

    const auto colors = {
        white_smoke, yellow, aqua, lime_green, blue_violet, indian_red};

    for (const auto& emu : emus)
    {
        for (const auto& type : types)
        {
            try
            {
                for (const auto& c : colors)
                    spdlog::info(fmt::format(fg(c), FMT_RUNTIME(m), emu.second, type.second));

                if (use_opldrv)
                    mid_test(emu.first, type.first, mixer, midi);
                else
                    scummvm_mid_test(emu.first, type.first, mixer, midi);
            }
            catch (const std::exception& e)
            {
                spdlog::default_logger()->error(e.what());
            }
        }
    }

    return 0;
}
