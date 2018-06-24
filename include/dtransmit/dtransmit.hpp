/* Copyright (C) ZJUDancer
 * 2018 - Yusu Pan <xxdsox@gmail.com>
 * 2017 - Wenxing Mei <mwx36mwx@gmail.com>
 */

/**
 * @file dtransmit.hpp
 * @author Yusu Pan, Wenxing Mei
 * @version 2018
 * @date 2018-03-04
 */

#pragma once
#include <ros/ros.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// http://wiki.ros.org/roscpp/Overview/MessagesSerializationAndAdaptingTypes
#define UDPBUFFERSIZE 65535

namespace dtransmit {

typedef int PORT;

// TODO maybe use pair
/**
 * @brief Wrapper for socket transmition
 */
struct Foo {
  //! Socket instance
  boost::asio::ip::udp::socket *socket;
  //! handler for receiving messages.
  std::function<void(const boost::system::error_code &, std::size_t)>
      readHandler;
  //! received buffer
  uint8_t recvBuffer[UDPBUFFERSIZE];
  //! remote endpoint address
  boost::asio::ip::udp::endpoint remoteEndpoint;

  /**
   * @brief Foo constructor (blank)
   */
  Foo() {}

  /**
   * @brief Foo constructor
   *
   * Wrapper of socket to listen on specified IP with reusable port.
   * https://stackoverflow.com/a/39665940
   *
   * @param service - io servise
   * @param port - port
   */
  Foo(boost::asio::io_service &service, PORT port) {
    // construct the socket
    socket = new boost::asio::ip::udp::socket(service);

    // open it
    boost::asio::ip::udp::endpoint rx_endpoint_(boost::asio::ip::udp::v4(),
                                                port);
    boost::system::error_code error;
    socket->open(rx_endpoint_.protocol(), error);
    if (error) {
      ROS_ERROR("Can't open recv socket");
    } else {
      // then set it for reuse and bind it
      socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
      socket->bind(rx_endpoint_, error);
      if (error) {
        ROS_ERROR("Can't bind recv socket");
      }
    }
  }

  /**
   * @brief Foo destructor
   */
  ~Foo() {}
};

/**
 * @brief Transmitting ROS messages and other information over UDP.
 */
class DTransmit {

 public:
  /**
   * @brief DTransmit constructor.
   *
   * @param address - udp boardcasting address
   */
  explicit DTransmit(std::string address);
  /**
   * @brief DTransmit destructor
   */
  ~DTransmit();

  /**
   * @brief Add listener for receiving ROS messages.
   *
   * @tparam ROSMSG - ROS message type
   * @param port - listening port
   * @param callback - callback function when receiving messages
   */
  template<typename ROSMSG>
  void addRosRecv(PORT port, std::function<void(ROSMSG &)> callback);

  /**
   * @brief Sending ROS messages.
   *
   * @tparam ROSMSG - ROS message type
   * @param port - sending port
   * @param ROSMSG - ROS message to send
   */
  template<typename ROSMSG>
  void sendRos(PORT port, ROSMSG &);

  /**
   * @brief Add listener for receiving raw messages.
   *
   * @param port - listening port
   * @param callback - callback function when receiving messages
   */
  void addRawRecv(PORT port, std::function<void(void *, std::size_t)> callback);

  /**
   * @brief Add listener for receiving raw messages from specific remote
   * endpoint.
   *
   * @param port - listening port
   * @param remoteEndpoint - given remote endpoint address
   * @param callback - callback function when receiving messages
   */
  void addRawRecvFiltered(PORT port, std::string remoteEndpoint,
                          std::function<void(void *, std::size_t)> callback);

  /**
   * @brief Send raw messages.
   *
   * @param port - gievn port
   * @param buffer - buffer to send
   * @param size - size of buffer
   */
  void sendRaw(PORT port, const void *buffer, std::size_t size);

  /**
   * @brief Start service of asio.
   */
  void startService();

 private:
  /**
   * @brief Start receiving.
   *
   * @tparam ReadHandler - type of handler
   * @param port - port for receiving messages
   * @param handler - handler for reading sockets
   */
  template<typename ReadHandler>
  void startRecv(PORT port, ReadHandler handler);
  /**
   * @brief Create socket for sending messages.
   *
   * @param PORT - port for sending messages
   */
  void createSendSocket(PORT);
  /**
   * @brief Send buffer.
   *
   * @param boost::asio::ip::udp::socket - socket
   * @param buffer - buffer to send
   * @param size - size of buffer
   */
  void sendBuffer(boost::asio::ip::udp::socket *, const void *buffer,
                  std::size_t size);

  //! UDP boardcast address
  std::string broadcast_address_;
  //! IO service
  boost::asio::io_service service_;

  //! Thread instance
  std::thread thread_;
  //! Map of ports and corresponding Foo for receiving messages
  std::unordered_map<PORT, Foo> recv_foo_;
  //! Map of ports and sockets for sending messages
  std::unordered_map<PORT, boost::asio::ip::udp::socket *> send_sockets_;
};

template<typename ReadHandler>
void DTransmit::startRecv(PORT port, ReadHandler handler) {
  recv_foo_[port].socket->async_receive_from(
      boost::asio::buffer(
          boost::asio::mutable_buffer((void *) &recv_foo_[port].recvBuffer,
                                      sizeof(recv_foo_[port].recvBuffer))),
      recv_foo_[port].remoteEndpoint, handler);
}

template<typename ROSMSG>
void DTransmit::addRosRecv(PORT port, std::function<void(ROSMSG &)> callback) {
  ROS_INFO("Add Ros Recv on port: %d", port);
  using namespace boost::asio;
  if (recv_foo_.count(port)) {
    ROS_ERROR("Error in addRosRecv: port %d exist!", port);
    return;
  }
  recv_foo_[port] = Foo(service_, port);

  recv_foo_[port].readHandler = [=](const boost::system::error_code &error,
                                    std::size_t bytesRecved) {
    if (error) {
      ROS_ERROR("Error in RosRecv: %s", error.message().c_str());
    } else {
      try {
        ROSMSG msg;

        ros::serialization::IStream stream((uint8_t *) recv_foo_[port].recvBuffer,
                                           bytesRecved);
        ros::serialization::Serializer<ROSMSG>::read(stream, msg);
        // client callback
        callback(msg);
      } catch (std::exception &e) {
        ROS_ERROR("%s", e.what());
      }
    }

    startRecv(port, recv_foo_[port].readHandler);
  };
  startRecv(port, recv_foo_[port].readHandler);
}

template<typename ROSMSG>
void DTransmit::sendRos(PORT port, ROSMSG &rosmsg) {
  try {
    // serialize rosmsg
    uint32_t serial_size = ros::serialization::serializationLength(rosmsg);
    // std::unique_ptr<uint8_t> buffer(new uint8_t[serial_size]);
    auto buffer = new uint8_t[serial_size];

    ros::serialization::OStream stream(buffer, serial_size);
    ros::serialization::serialize(stream, rosmsg);

    sendRaw(port, buffer, serial_size);
    //!? leak memory on exception
    delete[] buffer;
  } catch (std::exception &e) {
    ROS_ERROR("%s", e.what());
  }
}

}  // namespace dtransmit
