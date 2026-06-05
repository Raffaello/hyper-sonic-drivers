#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <HyperSonicDrivers/files/IMFFile.hpp>
#include <cstdint>
#include <memory>

namespace HyperSonicDrivers::files
{

void opl_callback()
{
    static int delta_ticks = 0;
    delta_ticks++;
}

TEST(IMFFile, cstorDefault)
{
    IMFFile f("../fixtures/02.imf");

    auto data = f.data();
    EXPECT_EQ(data.size(), 2084);
}

TEST(IMFFile, file_not_found)
{
    EXPECT_THROW(IMFFile f(""), std::system_error);
}


}    // namespace HyperSonicDrivers::files

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
