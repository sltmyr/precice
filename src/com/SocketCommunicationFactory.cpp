#ifndef PRECICE_NO_SOCKETS

#include "SocketCommunication.hpp"

#include "SocketCommunicationFactory.hpp"
#include "com/SharedPointer.hpp"


namespace precice {
namespace com {
SocketCommunicationFactory::SocketCommunicationFactory(
    unsigned short portNumber,
    bool reuseAddress,
    std::string const& networkName,
    std::string const& addressDirectory)
    : _portNumber(portNumber)
    , _reuseAddress(reuseAddress)
    , _networkName(networkName)
    , _addressDirectory(addressDirectory) {
  if (_addressDirectory.empty()) {
    _addressDirectory = ".";
  }
}

SocketCommunicationFactory::SocketCommunicationFactory(
    std::string const& addressDirectory)
    : SocketCommunicationFactory(0, false, "lo", addressDirectory) {
}

PtrCommunication SocketCommunicationFactory::newCommunication() {
  return PtrCommunication(new SocketCommunication(
      _portNumber, _reuseAddress, _networkName, _addressDirectory));
}

std::string
SocketCommunicationFactory::addressDirectory() {
  return _addressDirectory;
}
}
} // namespace precice, com

#endif // not PRECICE_NO_SOCKETS
