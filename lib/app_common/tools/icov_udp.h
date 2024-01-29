#pragma once

#include "icov_log.h"

#include <boost/asio.hpp>

namespace icov {
namespace udp {

//
// Send a string via UDP to the specified destination
// ip addresss at the specified port (point-to-point
// not broadcast)
//
inline bool send_udp_message(const std::string &message,
                             const std::string &destination_ip,
                             const unsigned short port) {

  ICOV_DEBUG << "Send '" << message << "' to '" << destination_ip << "'";

  using namespace boost::asio;

  io_service io_service;
  ip::udp::socket socket(io_service);
  // Create the remote endpoint using the destination ip address and
  // the target port number.  This is not a broadcast
  auto remote =
      ip::udp::endpoint(ip::address::from_string(destination_ip), port);
  try {

    // Open the socket, socket's destructor will
    // automatically close it.
    socket.open(boost::asio::ip::udp::v4());
    // And send the string... (synchronous / blocking)
    socket.send_to(buffer(message), remote);

  } catch (const boost::system::system_error &ex) {
    // Exception thrown!
    // Examine ex.code() and ex.what() to see what went wrong!
    ICOV_ERROR << "UDP send error: " << ex.what();
    return false;
  }
  return true;
}

class UdpReceiver {
  boost::asio::io_service io_service;
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint endpoint;
  bool _async;
  std::vector<char> out;

public:
  UdpReceiver(const std::string &ip_address, const int port, bool async)
      : socket(io_service), _async(async) {

    // Open socket & make/bind endpoint
    socket.open(boost::asio::ip::udp::v4());
    endpoint = boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(ip_address), port);
    socket.bind(endpoint);
    socket.non_blocking(_async);

    boost::asio::socket_base::receive_buffer_size option;
    socket.get_option(option);
    const int max_size = option.value();
    out.resize(max_size);
  }

  std::vector<char> receive_binary() {
    size_t bytesRead = 0;
    try {
      bytesRead = socket.receive(boost::asio::buffer(out));
    } catch (const boost::system::system_error &ex) {
      if (!(_async && ex.code() == boost::asio::error::would_block)) {
        ICOV_ERROR << "UDP error" << ex.what();
      }
      bytesRead = 0;
    } catch (...) {
      ICOV_ERROR << "UDP unknown error";
      bytesRead = 0;
    }

    return std::vector<char>(out.cbegin(), out.cbegin() + bytesRead);
  }

  //
  // Receive a UDP Datagram synchronously as a string
  // This function will block until a datagram
  // is received.
  //
  std::string receive() {
    const auto datagram = receive_binary();
    return std::string(datagram.begin(), datagram.end());
  }
};

class UdpSender {
  boost::asio::io_service io_service;
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint remote_endpoint;

public:
  UdpSender(const std::string &ip_address, const int port,
            const bool broadcast = false)
      : socket(io_service) {

    // Open socket
    socket.open(boost::asio::ip::udp::v4());

    if (broadcast) {
      boost::asio::socket_base::broadcast option(true);
      socket.set_option(option);
    }
    // make endpoint
    remote_endpoint = boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(ip_address.c_str()), port);
  }
  // Send a string to the preconfigured endpoint
  // via the open socket.
  void send(const std::string &message) {
    boost::system::error_code ignored_error;
    socket.send_to(boost::asio::buffer(message), remote_endpoint, 0,
                   ignored_error);
  }
  // Send some binary data to the preconfigured endpoint
  // via the open socket.
  void send(const unsigned char *data, const int len) {
    boost::system::error_code ignored_error;
    socket.send_to(boost::asio::buffer(data, len), remote_endpoint, 0,
                   ignored_error);
  }
};

} // namespace udp
} // namespace icov
