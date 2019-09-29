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

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "dtransmit/recv_socket.hpp"
#include <rmw/rmw.h>

namespace dtransmit {

/**
 * @brief Transmitting ROS messages and other information over UDP.
 */
class DTransmit {
 public:
  /**
   * @brief DTransmit constructor.
   *
   * @param address - udp broadcast address
   */
  explicit DTransmit(const std::string &address = "",
                     const bool &use_local_loop = false);
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
  template <typename ROSMSG>
  void addRosRecv(PORT port, std::function<void(ROSMSG &)> callback);

  /**
   * @brief Sending ROS messages.
   *
   * @tparam ROSMSG - ROS message type
   * @param port - sending port
   * @param ROSMSG - ROS message to send
   */
  template <typename ROSMSG>
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
  template <typename ReadHandler>
  void startRecv(PORT port, ReadHandler handler);
  /**
   * @brief Create socket for sending messages.
   *
   * @param addr - broadcast address for sending messages
   * @param port - port for sending messages
   */
  void createSendSocket(const std::string &addr, const PORT &port);
  /**
   * @brief Send buffer.
   *
   * @param boost::asio::ip::udp::socket - socket
   * @param buffer - buffer to send
   * @param size - size of buffer
   */
  void sendBuffer(boost::asio::ip::udp::socket *, const void *buffer,
                  std::size_t size);

  /**
   * @brief Retrieve for all interfaces the broadcast address
   */
  void retrieveBroadcastAddress();

  //! Broadcast addresses of all interfaces
  std::vector<std::string> broadcast_addresses_;
  //! IO service
  boost::asio::io_service service_;

  //! Thread instance
  std::thread thread_;
  //! Map of ports and corresponding Foo for receiving messages
  std::map<PORT, Foo> recv_foo_;
  //! Map of ports and sockets for sending messages
  std::map<std::pair<std::string, PORT>, boost::asio::ip::udp::socket *>
      send_sockets_;
};

template <typename ReadHandler>
void DTransmit::startRecv(PORT port, ReadHandler handler) {
  recv_foo_[port].socket->async_receive_from(
      boost::asio::buffer(
          boost::asio::mutable_buffer((void *)&recv_foo_[port].recvBuffer,
                                      sizeof(recv_foo_[port].recvBuffer))),
      recv_foo_[port].remoteEndpoint, handler);
}

template <typename ROSMSG>
void DTransmit::addRosRecv(PORT port, std::function<void(ROSMSG &)> callback) {
  std::cout << "Add Ros Recv on port: " << port;
  using namespace boost::asio;

  // if (recv_foo_.count(port)) {
  //   std::cerr << "Error in addRosRecv: port %d exist!" << port;
  //   return;
  // }
  // recv_foo_[port] = Foo(service_, port);

  // recv_foo_[port].readHandler = [=](const boost::system::error_code &error,
  //                                   std::size_t bytesRecved) {
  //   if (error) {
  //     std::cerr << "Error in RosRecv: " << error.message();
  //   } else {
  //     try {
  //       ROSMSG msg;

  //       ros::serialization::IStream stream(
  //           (uint8_t *)recv_foo_[port].recvBuffer, bytesRecved);
  //       ros::serialization::Serializer<ROSMSG>::read(stream, msg);
  //       // client callback
  //       callback(msg);
  //     } catch (std::exception &e) {
  //       std::cerr << e.what();
  //     }
  //   }

  //   startRecv(port, recv_foo_[port].readHandler);
  // };
  // startRecv(port, recv_foo_[port].readHandler);
}

template <typename ROSMSG>
void DTransmit::sendRos(PORT port, ROSMSG &rosmsg) {
  try {
    // // serialize rosmsg
    // uint32_t serial_size = ros::serialization::serializationLength(rosmsg);
    // // std::unique_ptr<uint8_t> buffer(new uint8_t[serial_size]);
    // auto buffer = new uint8_t[serial_size];

    // ros::serialization::OStream stream(buffer, serial_size);
    // ros::serialization::serialize(stream, rosmsg);

    // sendRaw(port, buffer, serial_size);
    // //!? leak memory on exception
    // delete[] buffer;
    rcl_serialized_message_t serialized_msg_ = rmw_get_zero_initialized_serialized_message();
    auto allocator = rcutils_get_default_allocator();
    auto initial_capacity = 0u;
    auto ret = rmw_serialized_message_init(&serialized_msg_, initial_capacity,
                                           &allocator);
    if (ret != RCL_RET_OK) {
      throw std::runtime_error("failed to initialize serialized message");
    }

    std::cout << "fuck" << std::endl;
  } catch (std::exception &e) {
    std::cerr << e.what();
  }
}

}  // namespace dtransmit
