#include <gtest/gtest.h>
#include "dtransmit/dbroadcast.hpp"
#include <ifaddrs.h>

using namespace dtransmit;

TEST(TestUDPBroadcast, testGetBroadcastIP) {
  auto _countSend = 0;
  std::vector<int> _broadcastAddr;

  struct ifaddrs *ifap;
  if (getifaddrs(&ifap) == -1) {
    std::cout <<
              "ERROR: UDPBroadcast: get broadcast address failed" << std::endl;
    std::cout << strerror(errno) << std::endl;
    return;
  }

  while (ifap != nullptr) {
    SOCKADDR *addr = ifap->ifa_broadaddr;
    if (addr != nullptr && addr->sa_family == AF_INET) {
      char str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(((SOCKADDR_IN *) addr)->sin_addr), str, INET_ADDRSTRLEN);
      printf("%s\n", str); // prints "192.0.2.33"
      _broadcastAddr.push_back(((SOCKADDR_IN *) addr)->sin_addr.s_addr);
    }
    ifap = ifap->ifa_next;
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
