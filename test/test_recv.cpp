#include <std_msgs/msg/string.hpp>
#include "dtransmit/dtransmit.hpp"

using namespace dtransmit;
using namespace std;

int main() {
  DTransmit d;
  int NUM = 2;

  int cnt = 0;
  for (int i = 0; i < NUM; ++i) {
    d.addRosRecv<std_msgs::msg::String>(2000 + i, [=, &cnt](std_msgs::msg::String &msg) {
      std::cout << 2000 + i << " heard: [" << msg.data << "]" << std::endl;
      // ROS_INFO("%d heard: [%s]", 2000 + i, msg.data.c_str());
      ++cnt;
    });
  }

  d.startService();

  while (cnt < 1000) {
    usleep(1000);
  }
}