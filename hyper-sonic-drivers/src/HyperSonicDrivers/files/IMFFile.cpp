#include <HyperSonicDrivers/files/IMFFile.hpp>

#include <HyperSonicDrivers/utils/ILogger.hpp>

#include <array>

namespace HyperSonicDrivers::files
{
using utils::logD;

IMFFile::IMFFile(
    const std::string& filename)
    : File(filename)
{
    uint16_t size = readLE16();
    if (size == 0)
        throw std::invalid_argument(std::format("file {} IMF Type-0 not supported", filename));

    size -= 2;
    if (this->size() < size)
        throw std::invalid_argument(std::format("file {} not IMF type", filename));

    std::array<bool, 9> used_channels;
    used_channels.fill(false);
    m_data.clear();
    for (uint32_t size_ = 0; size_ < size; size_ += 4)
    {
        IMF_Packet_t packet = {
            .reg         = readU8(),
            .val         = readU8(),
            .delay_ticks = readLE16(),
        };


        logD(std::format("reg[{}]={} (delay_ticks={})", packet.reg, packet.val, packet.delay_ticks));
        m_data.push_back(packet);
    }

    // tag data
    if (this->size() >= size + 2ULL + 16 + 64 + 6)
    {
        uint16_t a = readLE16();
        char     title[16]{};
        char     remarks[64]{};
        char     prog[6]{};

        for (int i = 0; i < 16; ++i)
            title[i] = readU8();
        for (int i = 0; i < 64; ++i)
            remarks[i] = readU8();
        for (int i = 0; i < 6; ++i)
            prog[i] = readU8();

        logD(std::format("a={}, title={}, remarks={}, prog={}", a, title, remarks, prog));
    }
}
}    // namespace HyperSonicDrivers::files
