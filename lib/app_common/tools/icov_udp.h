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
bool send_udp_message(const std::string &message,
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
} // namespace udp
} // namespace icov
