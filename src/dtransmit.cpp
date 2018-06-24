/* Copyright (C) ZJUDancer
 * 2018 - Yusu Pan <xxdsox@gmail.com>
 * 2017 - Wenxing Mei <mwx36mwx@gmail.com>
 */

/**
 * @file dtransmit.cpp
 * @author Yusu Pan, Wenxing Mei
 * @version 2018
 * @date 2018-03-04
 */

#include <algorithm>
#include "dtransmit/dtransmit.hpp"

using namespace std;
using namespace boost::asio;

namespace dtransmit {
DTransmit::DTransmit(string address)
    : broadcast_address_(address), service_() {
  std::cout << "New DTransmit on " << address << std::endl;
}

DTransmit::~DTransmit() {
  service_.stop();
  for (auto& p : send_sockets_) {
    ip::udp::socket* s = p.second;
    if (s) {
      if (s->is_open()) s->close();
      delete s;
    }
  }

  for (auto& p : recv_foo_) {
    ip::udp::socket* s = p.second.socket;
    if (s) {
      if (s->is_open()) s->close();

      delete s;
    }
  }

  service_.stop();
  if (thread_.joinable()) thread_.join();
}

void DTransmit::startService() {
  thread_ = std::thread([&]() {
    service_.reset();
    service_.run();
  });
}

void DTransmit::createSendSocket(PORT port) {
  using namespace boost::asio;
  ip::udp::endpoint broadcastEndpoint(
      ip::address::from_string(broadcast_address_), port);
  send_sockets_[port] = new ip::udp::socket(service_, ip::udp::v4());
  send_sockets_[port]->set_option(socket_base::broadcast(true));

  boost::system::error_code ec;
  send_sockets_[port]->connect(broadcastEndpoint, ec);
  if (ec) {
    ROS_ERROR("DTransmit create sendRos socket error: %s",
              ec.message().c_str());
  }
}

void DTransmit::sendBuffer(boost::asio::ip::udp::socket* socket,
                           const void* buffer, std::size_t size) {
  boost::system::error_code ec;
  socket->send(boost::asio::buffer(buffer, size), 0, ec);
  if (ec) {
    // ROS_WARN("DTransmit can't send Ros buffer: %s", ec.message().c_str());
  }
}

void DTransmit::addRawRecv(PORT port,
                           std::function<void(void*, std::size_t)> callback) {
  if (recv_foo_.count(port)) {
    ROS_ERROR("Error in addRawRecv: port %d exist!", port);
    return;
  }
  recv_foo_[port] = Foo(service_, port);

  recv_foo_[port].readHandler = [=](const boost::system::error_code& error,
                                    std::size_t bytesRecved) {
    if (error) {
      ROS_ERROR("Error in RosRecv: %s", error.message().c_str());
    } else {
      callback(recv_foo_[port].recvBuffer, bytesRecved);
    }

    startRecv(port, recv_foo_[port].readHandler);
  };
  startRecv(port, recv_foo_[port].readHandler);
}

void DTransmit::addRawRecvFiltered(
    PORT port, std::string remoteEndpoint,
    std::function<void(void*, std::size_t)> callback) {
  if (recv_foo_.count(port)) {
    ROS_ERROR("Error in addRawRecv: port %d exist!", port);
    return;
  }
  recv_foo_[port] = Foo(service_, port);

  recv_foo_[port].readHandler = [=](const boost::system::error_code& error,
                                    std::size_t bytesRecved) {
    if (error) {
      ROS_ERROR("Error in RosRecv: %s", error.message().c_str());
    } else if (recv_foo_[port].remoteEndpoint.address().to_string() !=
               remoteEndpoint) {
      // ROS_WARN("Someone else [%s] is broadcasting on this port [%d], packet
      // filtered.",
      // recv_foo_[port].remoteEndpoint.address().to_string().c_str(), port);
    } else {
      callback(recv_foo_[port].recvBuffer, bytesRecved);
    }

    startRecv(port, recv_foo_[port].readHandler);
  };
  startRecv(port, recv_foo_[port].readHandler);
}

void DTransmit::sendRaw(PORT port, const void* buffer, std::size_t size) {
  if (!send_sockets_.count(port)) {
    createSendSocket(port);
  }
  sendBuffer(send_sockets_[port], buffer, size);
}
}
