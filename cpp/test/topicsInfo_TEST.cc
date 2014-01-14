#include <limits.h>
#include "../topicsInfo.hh"
#include "gtest/gtest.h"

TEST(PacketTest, Basic)
{
  EXPECT_EQ(1, 1);
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
