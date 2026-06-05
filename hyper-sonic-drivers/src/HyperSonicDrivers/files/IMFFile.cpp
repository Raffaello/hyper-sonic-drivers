#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

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

    if (this->size() < size)
        throw std::invalid_argument(std::format("file {} not IMF type", filename));

    if (size % 4 != 0)
        throw std::invalid_argument(std::format("file {} has invalid IMF packet length", filename));

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
        m_tag_unknown = readLE16();
        m_title.resize(16);
        m_remarks.resize(64);
        m_prog.resize(6);

        for (int i = 0; i < 16; ++i)
            m_title[i] = readU8();
        for (int i = 0; i < 64; ++i)
            m_remarks[i] = readU8();
        for (int i = 0; i < 6; ++i)
            m_prog[i] = readU8();

        m_title.shrink_to_fit();
        m_remarks.shrink_to_fit();
        m_prog.shrink_to_fit();
        m_has_tag = true;

        logD(std::format("a={}, title={}, remarks={}, prog={}", m_tag_unknown, m_title, m_remarks, m_prog));
    }
}
}    // namespace HyperSonicDrivers::files
