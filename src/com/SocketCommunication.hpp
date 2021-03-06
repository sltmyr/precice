#ifndef PRECICE_NO_SOCKETS

#pragma once

#include "com/Communication.hpp"

#include "logging/Logger.hpp"
#include <boost/asio/io_service.hpp>

#include <thread>

namespace boost {
namespace asio {
namespace ip {
class tcp;
}
template <typename Protocol>
class stream_socket_service;
template <typename Protocol, typename StreamSocketService>
class basic_stream_socket;
}
namespace system {
class error_code;
}
}

namespace precice {
namespace com {
/// Implements Communication by using sockets.
class SocketCommunication : public Communication {
public:

  SocketCommunication(unsigned short portNumber = 0,
                      bool reuseAddress = false,
                      std::string const& networkName = "lo",
                      std::string const& addressDirectory = ".");


  explicit SocketCommunication(std::string const& addressDirectory);

  virtual ~SocketCommunication();

  /// Returns true, if a connection to a remote participant has been setup.
  virtual bool isConnected();

  /**
   * @brief Returns the number of processes in the remote communicator.
   *
   * Precondition: a connection to the remote participant has been setup.
   */
  virtual size_t getRemoteCommunicatorSize();

  /**
   * @brief Accepts connection from participant, which has to call
   *requestConnection().
   *
   * If several connections are going in to a server, the server has to call
   *this
   * method, while the clients have to call requestConnection().
   *
   * @param[in] nameAcceptor Name of calling participant.
   * @param[in] nameRequester Name of remote participant to connect to.
   */
  virtual void acceptConnection(std::string const& nameAcceptor,
                                std::string const& nameRequester,
                                int acceptorProcessRank,
                                int acceptorCommunicatorSize);

  virtual void acceptConnectionAsServer(std::string const& nameAcceptor,
                                        std::string const& nameRequester,
                                        int requesterCommunicatorSize);

  /**
   * @brief Requests connection from participant, which has to call
   *acceptConnection().
   *
   * If several connections are going in to a server, the clients have to call
   *this
   * method, while the server has to call acceptConnection().
   *
   * @param nameAcceptor [IN] Name of remote participant to connect to.
   * @param nameReuester [IN] Name of calling participant.
   */
  virtual void requestConnection(std::string const& nameAcceptor,
                                 std::string const& nameRequester,
                                 int requesterProcessRank,
                                 int requesterCommunicatorSize);

  virtual int requestConnectionAsClient(std::string const& nameAcceptor,
                                        std::string const& nameRequester);

  /**
   * @brief Disconnects from communication space, i.e. participant.
   *
   * This method is called on destruction.
   */
  virtual void closeConnection();

  /**
   * @brief Is empty.
   */
  virtual void startSendPackage(int rankReceiver);

  /**
   * @brief Is empty.
   */
  virtual void finishSendPackage();

  /**
   * @brief Just returns rank of sender.
   */
  virtual int startReceivePackage(int rankSender);

  /**
   * @brief Is empty.
   */
  virtual void finishReceivePackage();

  /**
   * @brief Sends a std::string to process with given rank.
   */
  virtual void send(std::string const& itemToSend, int rankReceiver);

  /**
   * @brief Sends an array of integer values.
   */
  virtual void send(int* itemsToSend, int size, int rankReceiver);

  /**
   * @brief Asynchronously sends an array of integer values.
   */
  virtual PtrRequest aSend(int* itemsToSend,
                                       int size,
                                       int rankReceiver);

  /**
   * @brief Sends an array of double values.
   */
  virtual void send(double* itemsToSend, int size, int rankReceiver);

  /**
   * @brief Asynchronously sends an array of double values.
   */
  virtual PtrRequest aSend(double* itemsToSend,
                                       int size,
                                       int rankReceiver);

  /**
   * @brief Sends a double to process with given rank.
   */
  virtual void send(double itemToSend, int rankReceiver);

  /**
   * @brief Asynchronously sends a double to process with given rank.
   */
  virtual PtrRequest aSend(double* itemToSend, int rankReceiver);

  /**
   * @brief Sends an int to process with given rank.
   */
  virtual void send(int itemToSend, int rankReceiver);

  /**
   * @brief Asynchronously sends an int to process with given rank.
   */
  virtual PtrRequest aSend(int* itemToSend, int rankReceiver);

  /**
   * @brief Sends a bool to process with given rank.
   */
  virtual void send(bool itemToSend, int rankReceiver);

  /**
   * @brief Asynchronously sends a bool to process with given rank.
   */
  virtual PtrRequest aSend(bool* itemToSend, int rankReceiver);

  /**
   * @brief Receives a std::string from process with given rank.
   */
  virtual void receive(std::string& itemToReceive, int rankSender);

  /**
   * @brief Receives an array of integer values.
   */
  virtual void receive(int* itemsToReceive, int size, int rankSender);

  /**
   * @brief Asynchronously receives an array of integer values.
   */
  virtual PtrRequest aReceive(int* itemsToReceive,
                                          int size,
                                          int rankSender);

  /**
   * @brief Receives an array of double values.
   */
  virtual void receive(double* itemsToReceive, int size, int rankSender);

  /**
   * @brief Asynchronously receives an array of double values.
   */
  virtual PtrRequest aReceive(double* itemsToReceive,
                                          int size,
                                          int rankSender);

  /**
   * @brief Receives a double from process with given rank.
   */
  virtual void receive(double& itemToReceive, int rankSender);

  /**
   * @brief Asynchronously receives a double from process with given rank.
   */
  virtual PtrRequest aReceive(double* itemToReceive,
                                          int rankSender);

  /**
   * @brief Receives an int from process with given rank.
   */
  virtual void receive(int& itemToReceive, int rankSender);

  /**
   * @brief Asynchronously receives an int from process with given rank.
   */
  virtual PtrRequest aReceive(int* itemToReceive, int rankSender);

  /// Receives a bool from process with given rank.
  virtual void receive(bool& itemToReceive, int rankSender);

  /// Asynchronously receives a bool from process with given rank.
  virtual PtrRequest aReceive(bool* itemToReceive, int rankSender);

private:
  static logging::Logger _log;

  // @brief Port used for socket connection.
  unsigned short _portNumber;

  bool _reuseAddress;

  /// Name of network to communicate over.
  std::string _networkName;

  /// Directory where IP address is exchanged by file.
  std::string _addressDirectory;

  bool _isConnected;

  int _remoteCommunicatorSize;

  typedef boost::asio::io_service IOService;
  std::shared_ptr<IOService> _ioService;

  typedef boost::asio::ip::tcp TCP;
  typedef boost::asio::stream_socket_service<TCP> SocketService;
  typedef boost::asio::basic_stream_socket<TCP, SocketService> Socket;
  typedef std::shared_ptr<Socket> PtrSocket;
  std::vector<PtrSocket> _sockets;

  typedef boost::asio::io_service::work Work;
  typedef std::shared_ptr<Work> PtrWork;
  PtrWork _work;

  std::thread _thread;

  bool isClient();
  bool isServer();

  std::string getIpAddress();
};

}} // namespace precice, com


#endif // not PRECICE_NO_SOCKETS
