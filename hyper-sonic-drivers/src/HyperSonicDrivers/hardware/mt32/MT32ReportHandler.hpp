#pragma once

#include <cstdint>
#include <cstdarg>
#include <mt32emu/c_interface/cpp_interface.h>

namespace HyperSonicDrivers::hardware::mt32
{
class MT32ReportHandler : public MT32Emu::IReportHandlerV1
{
public:
    MT32ReportHandler()  = default;
    ~MT32ReportHandler() = default;

    void printDebug(const char* fmt, va_list list) override;
    void onErrorControlROM() override;
    void onErrorPCMROM() override;
    void showLCDMessage(const char* message) override;
    void onMIDIMessagePlayed() override;
    bool onMIDIQueueOverflow() override;
    void onMIDISystemRealtime(uint8_t system_realtime) override;
    void onDeviceReset() override;
    void onDeviceReconfig() override;
    void onNewReverbMode(uint8_t mode) override;
    void onNewReverbTime(uint8_t time) override;
    void onNewReverbLevel(uint8_t level) override;
    void onPolyStateChanged(uint8_t part_num) override;
    void onProgramChanged(uint8_t part_num, const char* sound_group_name, const char* patch_name) override;

    void onLCDStateUpdated() override;
    void onMidiMessageLEDStateUpdated(bool ledState) override;
};
}    // namespace HyperSonicDrivers::hardware::mt32
