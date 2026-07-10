#include <HyperSonicDrivers/hardware/mt32/MT32ReportHandler.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::hardware::mt32
{
using utils::logT;
using utils::logD;
using utils::logW;
using utils::logI;
using utils::logE;

void MT32ReportHandler::printDebug(const char* fmt, va_list list)
{
    constexpr const int buf_size = 1024;
    std::string         buf;
    buf.resize(buf_size);

    vsnprintf(buf.data(), buf_size, fmt, list);
    logD(buf);
}

void MT32ReportHandler::onErrorControlROM()
{
    logE("Control ROM error");
}

void MT32ReportHandler::onErrorPCMROM()
{
    logE("PCM ROM error");
}

void MT32ReportHandler::showLCDMessage(const char* message)
{
    logI(std::format("MT32 LCD: {}", message));
}

void MT32ReportHandler::onMIDIMessagePlayed()
{
    logT("MidiMessagePlayed");
}

bool MT32ReportHandler::onMIDIQueueOverflow()
{
    logW("MidiQueue Overflow");
    return false;
}

void MT32ReportHandler::onMIDISystemRealtime(uint8_t system_realtime)
{
    logD(std::format("MIDI sys rt = {:d}", system_realtime));
}

void MT32ReportHandler::onDeviceReset()
{
    logD("device reset");
}

void MT32ReportHandler::onDeviceReconfig()
{
    logD("device reconfig");
}

void MT32ReportHandler::onNewReverbMode(uint8_t mode)
{
    logD(std::format("Reverb Mode = {}", mode));
}

void MT32ReportHandler::onNewReverbTime(uint8_t time)
{
    logD(std::format("Reverb Time = {}", time));
}

void MT32ReportHandler::onNewReverbLevel(uint8_t level)
{
    logD(std::format("Reverb Level = {}", level));
}

void MT32ReportHandler::onPolyStateChanged(uint8_t part_num)
{
    logD(std::format("Poly-state changed = {}", part_num));
}

void MT32ReportHandler::onProgramChanged(uint8_t part_num, const char* sound_group_name, const char* patch_name)
{
    logD(std::format("Program Changed = {} - {} - {}", part_num, sound_group_name, patch_name));
}

void MT32ReportHandler::onLCDStateUpdated()
{
    logD("LCD State Updated");
}

void MT32ReportHandler::onMidiMessageLEDStateUpdated(bool ledState)
{
    logD("Midi Message LED State Updated");
}
}    // namespace HyperSonicDrivers::hardware::mt32
