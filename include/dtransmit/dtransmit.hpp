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

#include <rmw/rmw.h>
#include <std_msgs/msg/string.hpp>
#include "dtransmit/recv_socket.hpp"

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
  explicit DTransmit(const std::string &address = "", const bool &use_local_loop = false);
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
  void sendBuffer(boost::asio::ip::udp::socket *, const void *buffer, std::size_t size);

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
  std::map<std::pair<std::string, PORT>, boost::asio::ip::udp::socket *> send_sockets_;
};

template <typename ReadHandler>
void DTransmit::startRecv(PORT port, ReadHandler handler) {
  recv_foo_[port].socket->async_receive_from(
      boost::asio::buffer(boost::asio::mutable_buffer((void *)&recv_foo_[port].recvBuffer,
                                                      sizeof(recv_foo_[port].recvBuffer))),
      recv_foo_[port].remoteEndpoint, handler);
}

template <typename ROSMSG>
void DTransmit::addRosRecv(PORT port, std::function<void(ROSMSG &)> callback) {
  std::cout << "Add Ros Recv on port: " << port;
  using namespace boost::asio;

  if (recv_foo_.count(port)) {
    std::cerr << "Error in addRosRecv: port %d exist!" << port;
    return;
  }
  recv_foo_[port] = Foo(service_, port);

  recv_foo_[port].readHandler = [=](const boost::system::error_code &error,
                                    std::size_t bytesRecved) {
    if (error) {
      std::cerr << "Error in RosRecv: " << error.message();
    } else {
      try {
        ROSMSG msg;

        // Init aserialized message
        rcl_serialized_message_t serialized_msg_ = rmw_get_zero_initialized_serialized_message();
        auto allocator = rcutils_get_default_allocator();
        auto initial_capacity = 8u;
        auto ret = rmw_serialized_message_init(&serialized_msg_, initial_capacity, &allocator);
        if (ret != RCL_RET_OK) {
          throw std::runtime_error("failed to initialize serialized message");
        }

        ret = rmw_serialized_message_resize(&serialized_msg_, bytesRecved);
        if (ret != RCL_RET_OK) {
          throw std::runtime_error("failed to resize serialized message");
        }
        // Get msg content from buffer
        memcpy((void *)serialized_msg_.buffer, (void *)recv_foo_[port].recvBuffer, bytesRecved);
        serialized_msg_.buffer_length = bytesRecved;

        auto string_ts =
            rosidl_typesupport_cpp::get_message_type_support_handle<ROSMSG>();

        // Deserialize message into ros msg
        ret = rmw_deserialize(&serialized_msg_, string_ts, &msg);
        if (ret != RMW_RET_OK) {
          fprintf(stderr, "failed to deserialize serialized message\n");
          return;
        }
        // // client callback
        callback(msg);
      } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
      }
    }

    startRecv(port, recv_foo_[port].readHandler);
  };
  startRecv(port, recv_foo_[port].readHandler);
}

template <typename ROSMSG>
void DTransmit::sendRos(PORT port, ROSMSG &rosmsg) {
  try {
    // Init empty serialize message
    rcl_serialized_message_t serialized_msg_ = rmw_get_zero_initialized_serialized_message();
    auto allocator = rcutils_get_default_allocator();
    auto initial_capacity = 0u;
    auto ret = rmw_serialized_message_init(&serialized_msg_, initial_capacity, &allocator);
    if (ret != RCL_RET_OK) {
      throw std::runtime_error("failed to initialize serialized message");
    }

    // auto message_header_length = 8u;
    // auto message_payload_length = static_cast<size_t>(rosmsg.data.size());
    // ret = rmw_serialized_message_resize(&serialized_msg_,
    //                                     message_header_length + message_payload_length);
    if (ret != RCL_RET_OK) {
      throw std::runtime_error("failed to resize serialized message");
    }

    // serialize rosmsg
    auto string_ts =
        rosidl_typesupport_cpp::get_message_type_support_handle<ROSMSG>();
    // Given the correct typesupport, we can convert our ROS2 message into
    // its binary representation (serialized_msg)
    ret = rmw_serialize(&rosmsg, string_ts, &serialized_msg_);
    if (ret != RMW_RET_OK) {
      fprintf(stderr, "failed to serialize serialized message\n");
      return;
    }
    sendRaw(port, serialized_msg_.buffer, serialized_msg_.buffer_length);

  } catch (std::exception &e) {
    std::cerr << e.what();
  }
}

}  // namespace dtransmit
