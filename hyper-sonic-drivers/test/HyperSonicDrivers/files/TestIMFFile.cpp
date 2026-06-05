#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <HyperSonicDrivers/files/IMFFile.hpp>

namespace HyperSonicDrivers::files
{

TEST(IMFFile, cstorDefault)
{
    IMFFile f("../fixtures/02.imf");

    auto data = f.data();
    EXPECT_EQ(data.size(), 2084);
    EXPECT_TRUE(f.has_tag());
    EXPECT_EQ(1, f.tag_unknown());
    EXPECT_STREQ("WONDERIN", f.title().c_str());
    EXPECT_STREQ("\\sound\\WONDERIN.IMF", f.remarks().c_str());
    EXPECT_STREQ("\xE4\x13Y\x18\x92 ", f.prog().c_str());
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
