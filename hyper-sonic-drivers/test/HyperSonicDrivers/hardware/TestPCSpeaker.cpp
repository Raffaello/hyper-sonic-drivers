#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <HyperSonicDrivers/hardware/PCSpeaker.hpp>

namespace HyperSonicDrivers::hardware
{
TEST(PCSpeaker, cstorDefault)
{
    PCSpeaker pcSpeaker;
    EXPECT_EQ(pcSpeaker.getRate(), 44100);
    EXPECT_EQ(pcSpeaker.getChannels(), 2);
    EXPECT_EQ(pcSpeaker.getBits(), 16);
    EXPECT_EQ(pcSpeaker.getSigned(), true);
}

TEST(PCSpeaker, cstor)
{
    PCSpeaker pcSpeaker(1, 2, 3, false);
    EXPECT_EQ(pcSpeaker.getRate(), 1);
    EXPECT_EQ(pcSpeaker.getChannels(), 2);
    EXPECT_EQ(pcSpeaker.getBits(), 3);
    EXPECT_EQ(pcSpeaker.getSigned(), false);
}

TEST(DISABLED_PCSpeaker, cstor_invalid_parameters)
{
    FAIL();
}

TEST(PCSpeaker, isPlaying)
{
    PCSpeaker pcSpeaker(44100, 8);
    pcSpeaker.play(PCSpeaker::eWaveForm::SINE, 440, 1);
    EXPECT_TRUE(pcSpeaker.isPlaying());
}

TEST(PCSpeaker, readBuffer)
{
    int16_t   buf[1024];
    PCSpeaker pcSpeaker(44100, 2);
    pcSpeaker.play(PCSpeaker::eWaveForm::SINE, 440, 1);
    uint32_t ns         = 100;
    uint32_t numSamples = pcSpeaker.readBuffer<int16_t>(buf, ns);
    EXPECT_EQ(44100 * 2 * 1 / 1000, numSamples);
}

template <class T>
class Duration : public ::testing::TestWithParam<std::tuple<uint32_t, uint8_t>>
{
public:
    Duration()
    {
        freq      = std::get<0>(GetParam());
        channels  = std::get<1>(GetParam());
        pcSpeaker = new PCSpeaker(freq, channels, sizeof(T) * 8);
        readbuf_  = std::bind(&PCSpeaker::readBuffer<T>, pcSpeaker, buf, ns);
    }

    uint32_t readbuf()
    {
        return pcSpeaker->readBuffer<T>(buf, ns);
    }

protected:
    PCSpeaker*                                 pcSpeaker;
    std::function<uint32_t(T, const uint32_t)> readbuf_;
    T                                          buf[1024];
    uint32_t                                   ns   = 100;
    int32_t                                    time = 1000;
    uint32_t                                   freq;
    uint8_t                                    channels;

    void test_()
    {
        this->pcSpeaker->play(PCSpeaker::eWaveForm::SINE, 440, this->time);
        double   tsum = 0.0;
        uint32_t dsum = 0;

        while (pcSpeaker->isPlaying())
        {
            uint32_t d  = this->readbuf();
            dsum       += d;
            double t    = (double) d * 1000.0 / (double) this->pcSpeaker->getChannels() / (double) this->pcSpeaker->getRate();
            tsum       += t;
        }

        EXPECT_NEAR((double) this->time, tsum, 0.01);
        EXPECT_EQ(dsum, this->freq * this->time * this->channels / 1000);
    }
};

class DurationS8 : public Duration<int8_t>
{
};

TEST_P(DurationS8, duration_8bits)
{
    this->test_();
}

class DurationS16 : public Duration<int16_t>
{
};

TEST_P(DurationS16, duration_16bits)
{
    this->test_();
}

// TODO int32_t, float
/*class DurationS32 : public Duration<int32_t> {};
TEST_P(DurationS32, duration_32bits)
{
    this->test_();
}*/

template <typename T, std::size_t... I>
auto values(T* t, std::index_sequence<I...>)
{
    return ::testing::Values(t[I]...);
}

auto make_duration_values()
{
    constexpr int        nf           = 8;
    constexpr int        nc           = 7;
    const int            freq[nf]     = {11025, 22050, 44100, 48000, 88200, 96000, 176400, 192000};
    const int            channels[nc] = {1, 2, 4, 5, 6, 7, 8};
    std::tuple<int, int> res[nf * nc];
    int                  i = 0;
    for (auto& f : freq)
    {
        for (auto& c : channels)
        {
            res[i++] = std::make_tuple<>(f, c);
        }
    }

    return values(res, std::make_index_sequence<nf * nc>{});
}

INSTANTIATE_TEST_SUITE_P(
    PCSpeaker,
    DurationS8,
    make_duration_values());
INSTANTIATE_TEST_SUITE_P(
    PCSpeaker,
    DurationS16,
    make_duration_values());

/*INSTANTIATE_TEST_SUITE_P(
    PCSpeaker,
    DurationS32,
    make_duration_values()
);*/

TEST(DISABLED_PCSpeaker, callback)
{
    FAIL();
}
}    // namespace HyperSonicDrivers::hardware

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
