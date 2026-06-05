#pragma once

#include <HyperSonicDrivers/files/File.hpp>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace HyperSonicDrivers::files
{
class IMFFile : protected File
{
public:
    IMFFile(const std::string& filename);
    ~IMFFile() override = default;

    typedef struct IMF_Packet_t
    {
        uint8_t  reg;
        uint8_t  val;
        uint16_t delay_ticks;

    } IMF_Packet_t;

    const std::vector<IMF_Packet_t>& data() const noexcept;
    bool                             has_tag() const noexcept;
    const uint16_t                   tag_unknown() const noexcept;
    const std::string&               title() const noexcept;
    const std::string&               remarks() const noexcept;
    const std::string&               prog() const noexcept;

private:
    std::vector<IMF_Packet_t> m_data;

    bool        m_has_tag     = false;
    uint16_t    m_tag_unknown = 0;
    std::string m_title;
    std::string m_remarks;
    std::string m_prog;
};

inline bool IMFFile::has_tag() const noexcept
{
    return m_has_tag;
}

inline const std::vector<IMFFile::IMF_Packet_t>& IMFFile::data() const noexcept
{
    return m_data;
}

inline const uint16_t IMFFile::tag_unknown() const noexcept
{
    return m_tag_unknown;
}

inline const std::string& IMFFile::title() const noexcept
{
    return m_title;
}

inline const std::string& IMFFile::remarks() const noexcept
{
    return m_remarks;
}

inline const std::string& IMFFile::prog() const noexcept
{
    return m_prog;
}


}    // namespace HyperSonicDrivers::files
