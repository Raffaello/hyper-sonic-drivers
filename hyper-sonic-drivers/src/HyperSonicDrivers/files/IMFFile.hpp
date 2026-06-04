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

    inline const std::vector<IMF_Packet_t>& data() noexcept;

private:
    std::vector<IMF_Packet_t> m_data;
};

inline const std::vector<IMFFile::IMF_Packet_t>& IMFFile::data() noexcept
{
    return m_data;
}

}    // namespace HyperSonicDrivers::files
