#include <format>
#include <vector>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/audio/streams/EmulatedStream.hpp>
#include <HyperSonicDrivers/files/File.hpp>
#include <HyperSonicDrivers/hardware/mt32/MT32.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>
#include <std/MT32EmuReturnCode.hpp>

namespace HyperSonicDrivers::hardware::mt32
{
namespace fs = std::filesystem;

MT32::MT32(const std::filesystem::path& control_rom, const std::filesystem::path& pcm_rom, const std::shared_ptr<audio::IMixer>& mixer) : IHardware(mixer)
{
    m_service.createContext(m_rh);

    utils::logI(std::format("MUNT version: {}", m_service.getLibraryVersionString()));

    mt32emu_return_code ret = m_service.addROMFile(control_rom.string().c_str());
    if (ret != MT32EMU_RC_ADDED_CONTROL_ROM)
        utils::throwLogC<std::runtime_error>(std::format("can't add control ROM data (ret_code: {})", ret));

    ret = m_service.addROMFile(pcm_rom.string().c_str());
    if (ret != MT32EMU_RC_ADDED_PCM_ROM)
        utils::throwLogC<std::runtime_error>(std::format("can't add pcm ROM data (ret_code: {})", ret));

    mt32emu_rom_info info;
    m_service.getROMInfo(&info);
    utils::logI(std::format("Control ROM info: ID={}, desc={}, sha1={}", info.control_rom_id, info.control_rom_description, info.control_rom_sha1_digest));
    utils::logI(std::format("pcm     ROM info: ID={}, desc={}, sha1={}", info.pcm_rom_id, info.pcm_rom_description, info.pcm_rom_sha1_digest));

    reset();
}

MT32::~MT32()
{
    if (m_init)
    {
        m_service.closeSynth();
        m_init = false;
    }

    m_service.freeContext();
}

bool MT32::init()
{
    if (m_init)
        return true;

    mt32emu_return_code ret = m_service.openSynth();
    if (ret != MT32EMU_RC_OK)
    {
        utils::logC(std::format("can't open MT32 emulation device (ret code: {})", ret));
        return false;
    }

    m_output_rate = m_service.getActualStereoOutputSamplerate();
    utils::logI(std::format("MT32 output_rate = {}", m_output_rate));

    m_init = true;
    return true;
}

void MT32::reset()
{
    // TODO: need a way to change these parameters as user request
    m_service.setAnalogOutputMode(MT32Emu::AnalogOutputMode_ACCURATE);
    m_service.setStereoOutputSampleRate(m_mixer->freq);
    m_service.setSamplerateConversionQuality(MT32Emu::SamplerateConversionQuality_BEST);
    m_service.setOutputGain(1.0f);
    m_service.setReverbOutputGain(1.0f);
    m_service.selectRendererType(MT32Emu::RendererType_BIT16S);
    m_service.setMIDIDelayMode(MT32Emu::MIDIDelayMode_IMMEDIATE);
}

void MT32::start(
    const std::shared_ptr<TimerCallBack>& callback,
    const audio::mixer::eChannelGroup     group,
    const uint8_t                         volume,
    const uint8_t                         pan,
    const int                             timerFrequency)
{
    IHardware::start(callback, group, volume, pan, timerFrequency);
}

void MT32::sysEx(const uint32_t addr, const uint8_t* data, const uint32_t dataSize)
{
    std::vector<uint8_t> msg{
        static_cast<uint8_t>(audio::midi::MIDI_META_EVENT_VAL::SYS_EX0),
        0x41,
        0x10,
        0x16,
        0x12};
    msg.reserve(msg.size() + 4 + dataSize + 1);

    uint8_t checkSum = 0;

    uint8_t v = (addr >> 14) & 0x7F;
    msg.push_back(v);
    checkSum -= v;

    v         = (addr >> 7) & 0x7F;
    checkSum -= v;
    msg.push_back(v);

    v         = addr & 0x7F;
    checkSum -= v;
    msg.push_back(v);

    for (uint32_t i = 0; i < dataSize; i++)
    {
        checkSum -= *data;
        msg.push_back(*data++);
    }

    msg.push_back(checkSum & 0x7F);
    msg.push_back(static_cast<uint8_t>(audio::midi::MIDI_META_EVENT_VAL::SYS_EX7));

    m_service.playSysex(msg.data(), msg.size());
}

void MT32::startCallback(
    const audio::mixer::eChannelGroup group,
    const uint8_t                     volume,
    const uint8_t                     pan,
    const int                         timerFrequency)
{
    // TODO: this could be put in the parent class....
    setAudioStream(std::make_shared<audio::streams::EmulatedStream>(
        this,
        isStereo(),
        m_mixer->freq,
        setCallbackFrequency(timerFrequency)));

    m_channelId = m_mixer->play(
        group,
        getAudioStream(),
        volume,
        pan);

    if (!m_channelId.has_value())
    {
        utils::logC("can't start opl playback");
    }
}

void MT32::generateSamples(int16_t* buffer, const size_t length) noexcept
{
    m_service.renderBit16s(buffer, static_cast<uint32_t>(length));
}
}    // namespace HyperSonicDrivers::hardware::mt32
