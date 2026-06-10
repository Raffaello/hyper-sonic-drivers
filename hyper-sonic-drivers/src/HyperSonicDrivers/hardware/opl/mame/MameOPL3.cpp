#include <HyperSonicDrivers/hardware/opl/mame/MameOPL3.hpp>
#include <cstring>

namespace HyperSonicDrivers::hardware::opl::mame
{
using namespace opl3;

constexpr int OPL2_INTERNAL_FREQ = 3600000;     // The OPL2 operates at 3.6MHz;
constexpr int OPL3_INTERNAL_FREQ = 14400000;    // The OPL3 operates at 14.4MHz;

MameOPL3::MameOPL3(const OplType type, const std::shared_ptr<audio::IMixer>& mixer) : OPL(mixer, type),
                                                                                      _opl(nullptr), _chip(nullptr)
{
}

MameOPL3::~MameOPL3()
{
    if (_chip != nullptr)
    {
        stopCallback();
        // delete _opl;
        ymf262_shutdown(_chip);
    }

    if (_opl != nullptr)
    {
        stopCallback();
        delete _opl;
    }
}

bool MameOPL3::init()
{
    if (m_init)
    {
        return true;
    }

    _opl = new ymfm::ymf262(_ymfm);

    [[maybe_unused]] auto rate = _opl->sample_rate(OPL3_INTERNAL_FREQ);
    _opl->sample_rate(m_mixer->freq);

    _chip = ymf262_init(0, OPL3_INTERNAL_FREQ, m_mixer->freq);
    //_init = _opl != nullptr;
    m_init = _chip != nullptr;

    return m_init;
}

void MameOPL3::reset()
{
    //_opl->reset();
    ymf262_reset_chip(_chip);
}

void MameOPL3::write(const uint32_t port, const uint16_t val) noexcept
{
    // ???
    //_opl->write_address(a);
    //_opl->write_data(v);
    //_opl->write(port, val);

    ymf262_write(_chip, port, val);
}

uint8_t MameOPL3::read(const uint32_t port) noexcept
{
    // return _opl->read(port);

    return ymf262_read(_chip, port);
}

void MameOPL3::writeReg(const uint16_t r, const uint16_t v) noexcept
{
    //_opl->write(0, r);
    //_opl->write(1, v);

    ymf262_write(_chip, 0, r);
    ymf262_write(_chip, 1, v);
}

void MameOPL3::generateSamples(int16_t* buffer, const size_t length) noexcept
{
    constexpr int                              MAX_SIZE = 512;
    [[maybe_unused]] ymfm::ymf262::output_data b[MAX_SIZE];
    // int remaining = length * 2 / _opl->OUTPUTS;
    int remaining = length;

    int16_t  buf[4][MAX_SIZE];
    int16_t* buffers[4] = {buf[0], buf[1], buf[2], buf[3]};

    std::memset(buffer, 0, length * sizeof(int16_t));
    while (remaining > 0)
    {
        const int numSamples = std::min(remaining, MAX_SIZE);
        // or using _opl object or the c api yaml262_* functions.
        //_opl->generate(b, numSamples);

        ymf262_update_one(_chip, buffers, numSamples);
        // Interleave the samples before mixing
        for (int i = 0; i < numSamples; i++)
        {
            buffer[i * 2]     = buffers[0][i];
            buffer[i * 2 + 1] = buffers[1][i];
            buffer[i * 4 + 2] = buffers[2][i];
            buffer[i * 4 + 3] = buffers[3][i];

            // buffer[i + 0] = b[i].data[0];
            // buffer[i * 2 + 1] = b[i].data[1];
            // buffer[i * 2 + 2] = b[i].data[2] * 128;
            // buffer[i * 2 + 3] = b[i].data[3] * 128;

            // auto a = buf[0][i] - b[i].data[0];
            // auto z = buf[1][i] - b[i].data[1];
            // auto c = buf[2][i] - b[i].data[2];
            // auto d = buf[3][i] - b[i].data[3];
            // int e = 0;
        }

        // for (int i = 0; i < numSamples; i++) {
        //     //b[i].roundtrip_fp();
        //     /*for (int j = 0; j < _opl->OUTPUTS/2; j++) {
        //         auto a = b[i].data[j];
        //         buffer[i + j] = b[i].data[j];
        //     }*/

        // buffer[i*2] = b[i].data[0];
        // buffer[i*2 + 1] = b[i].data[1];
        // auto a = b[i];
        // //if (a.data[0] != 0 ||
        // //    a.data[1] != 0 ||
        // //    a.data[2] != 0 ||
        // //    a.data[3] != 0)
        // //{
        // //    //spdlog::info("MameOPL: b[{}]= [ {}, {}, {}, {} ]", i, a.data[0], a.data[1], a.data[2], a.data[3]);
        // //}
        // }

        remaining -= numSamples;
    }
}
}    // namespace HyperSonicDrivers::hardware::opl::mame
