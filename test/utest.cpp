#include <gtest/gtest.h>
#include "dtransmit/dtransmit.hpp"

using namespace dtransmit;

TEST(TestDTransmit, testGetBroadcastIP) {
  DTransmit transmitter("255.255.255.255");
  transmitter.addRawRecv(12345, [&](const void* buffer, std::size_t size){});
  transmitter.startService();

  struct Foo {
    int x = 1;
    double y = 2;
    std::string s = "hello";
  } foo;

  transmitter.sendRaw(2333, (void*)&foo, sizeof(foo));
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
