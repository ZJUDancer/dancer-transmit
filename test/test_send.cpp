#include <std_msgs/msg/string.hpp>
#include "dtransmit/dtransmit.hpp"

// TODO(MWX): test on different machine, over wifi
using namespace dtransmit;
using namespace std;

int main() {
  DTransmit d;

  int NUM = 2;

  for (int i = 0; i < 100; ++i) {
    std_msgs::msg::String msg;
    std::stringstream ss;
    ss << "Hello dtransmit " << std::to_string(i);
    msg.data = ss.str();

    std::cout << "Sending: [" << msg.data << "]" << std::endl;

    for (int j = 0; j < NUM; ++j) {
      d.sendRos<std_msgs::msg::String>(2000 + j, msg);
    }
    usleep(1000);
  }
}