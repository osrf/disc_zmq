#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <map>
#include <string>

#include "sockets/socket.hh"
#include "transport_defs.hh"
#include "zmq/zmq.hpp"
#include "zmq/zmsg.hpp"

const int MAXRCVSTRING = 65536; // Longest string to receive

class Node
{
  public:

    //  ---------------------------------------------------------------------
    /// \brief Constructor.
    /// \param[in] master End point with the master's endpoint.
    /// \param[in] _verbose true for enabling verbose mode.
    Node (std::string master, bool _verbose)
    {
      // Initialize random seed
      srand (time(NULL));

      // Required 0mq minimum version
      s_version_assert(2, 1);

      this->master = master;
      this->context = new zmq::context_t(1);
      this->verbose = _verbose;
      this->timeout = 2500000;           // msecs
      this->publisher = 0;
      this->subscriber = 0;

      // ToDo Read this from getenv
      this->broadcastAddress = "255.255.255.255";
      this->broadcastPort = 11312;
      this->broadcastSocket = new UDPSocket(this->broadcastPort);
      this->hostAddress = this->DetermineHost();

      if (this->verbose)
        std::cout << "Current host address: " << this->hostAddress << std::endl;

      s_catch_signals();
      connect_to_master();

      // Create the discovery thread
      this->discoveryThread = boost::thread(&Node::RecvDiscoveryUpdates, this);
    }

    //  ---------------------------------------------------------------------
    /// \brief Destructor.
    virtual ~Node()
    {
      if (this->publisher)
        delete this->publisher;
      if (this->subscriber)
        delete this->subscriber;
      if (this->context)
        delete this->context;
    }

    std::string DetermineHost()
    {
      char *ip_env;
      // First, did the user set ROS_HOSTNAME?
      ip_env = getenv("DZMQ_IP");

      if (ip_env) {
        if (this->verbose)
          std::cout << "determineHost: using value of DZMQ_IP: "
                    << ip_env << std::endl;

        if (strlen(ip_env) == 0)
        {
          std::cerr << "invalid DZMQ_IP (an empty string)" << std::endl;
        }
        return ip_env;
      }

      // Second, try the hostname
      char host[1024];
      memset(host,0,sizeof(host));
      if (gethostname(host, sizeof(host) - 1) != 0)
      {
        if (this->verbose)
          std::cerr << "determineIP: gethostname failed" << std::endl;
      }
      // We don't want localhost to be our ip
      else if(strlen(host) && strcmp("localhost", host))
      {
        return std::string(host);
      }

      // Third, fall back on interface search, which will yield an IP address

    #ifdef HAVE_IFADDRS_H
      struct ifaddrs *ifa = NULL, *ifp = NULL;
      int rc;
      if ((rc = getifaddrs(&ifp)) < 0)
      {
       std::cerr << "error in getifaddrs: " << strerror(rc) << std::endl;
       exit -1;
      }
      char preferred_ip[200] = {0};
      for (ifa = ifp; ifa; ifa = ifa->ifa_next)
      {
        char ip_[200];
        socklen_t salen;
        if (!ifa->ifa_addr)
          continue; // evidently this interface has no ip address
        if (ifa->ifa_addr->sa_family == AF_INET)
          salen = sizeof(struct sockaddr_in);
        else if (ifa->ifa_addr->sa_family == AF_INET6)
          salen = sizeof(struct sockaddr_in6);
        else
          continue;
        if (getnameinfo(ifa->ifa_addr, salen, ip_, sizeof(ip_), NULL, 0,
                        NI_NUMERICHOST) < 0)
        {
          if (this->verbose)
            std::cout << "getnameinfo couldn't get the ip of interface " <<
                         ifa->ifa_name << std::endl;
          continue;
        }

        // prefer non-private IPs over private IPs
        if (!strcmp("127.0.0.1", ip_) || strchr(ip_,':'))
          continue; // ignore loopback unless we have no other choice
        if (ifa->ifa_addr->sa_family == AF_INET6 && !preferred_ip[0])
          strcpy(preferred_ip, ip_);
        else if (isPrivateIP(ip_) && !preferred_ip[0])
          strcpy(preferred_ip, ip_);
        else if (!isPrivateIP(ip_) &&
                 (isPrivateIP(preferred_ip) || !preferred_ip[0]))
          strcpy(preferred_ip, ip_);
      }
      freeifaddrs(ifp);
      if (!preferred_ip[0])
      {
        std::cerr <<
          "Couldn't find a preferred IP via the getifaddrs() call; "
          "I'm assuming that your IP "
          "address is 127.0.0.1.  This should work for local processes, "
          "but will almost certainly not work if you have remote processes."
          "Report to the disc-zmq development team to seek a fix." << std::endl;
        return std::string("127.0.0.1");
      }
      if (this->verbose)
        std::cerr << "preferred IP is guessed to be " << preferred_ip << "\n";
      return std::string(preferred_ip);
    #else
      // @todo Fix IP determination in the case where getifaddrs() isn't
      // available.
      std::cerr <<
        "You don't have the getifaddrs() call; I'm assuming that your IP "
        "address is 127.0.0.1.  This should work for local processes, "
        "but will almost certainly not work if you have remote processes."
        "Report to the disc-zmq development team to seek a fix." << std::endl;
      return std::string("127.0.0.1");
    #endif
    }

    //  ---------------------------------------------------------------------
    /// \brief Thread in charge of receiving the discovery updates.
    void RecvDiscoveryUpdates()
    {
      while (true)
      {
        try {
          char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
          std::string sourceAddress;         // Address of datagram source
          unsigned short sourcePort;         // Port of datagram source
          int bytesRcvd = this->broadcastSocket->recvFrom(recvString,
            MAXRCVSTRING, sourceAddress, sourcePort);
          recvString[bytesRcvd] = '\0';      // Terminate string

          if (this->verbose)
          {
            cout << "Received " << recvString << " from " << sourceAddress <<
                    ": " << sourcePort << endl;
          }

          if (this->DispatchDiscoveryMsg(recvString) != 0)
            std::cerr << "Something went wrong parsing a discovery message\n";

        } catch (SocketException &e) {
          cerr << e.what() << endl;
        }
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Parse a discovery message received via the UDP broadcast socket.
    /// \param[in] _msg Received message.
    int DispatchDiscoveryMsg(char *_msg)
    {
      char *p;

      // Header fields
      uint8_t opCode;
      uint16_t bodyLength;

      // Body fields
      uint16_t topicLength;
      char *topic;
      char *guid;
      uint16_t addressesLength;
      char *addresses;

      std::vector<std::string>::iterator it;

      // Read the operation code
      p = _msg;
      memcpy(&opCode, p, sizeof(opCode));
      p += sizeof(opCode);

      // Read the body length
      memcpy(&bodyLength, p, sizeof(bodyLength));
      p += sizeof(bodyLength);

      if (this->verbose)
      {
        std::cout << "\n--- DT: I received a new message ---\n";
        std::cout << "Header:" << std::endl;
        std::cout << "\tOperation code: " << opCode << std::endl;
        std::cout << "\tBody length: " << bodyLength << std::endl;
      }

      switch (opCode)
      {
        case ADVERTISE:
          // Read the topic length
          memcpy(&topicLength, p, sizeof(topicLength));
          p += sizeof(topicLength);

          // Read the topic
          topic = new char(topicLength);
          memcpy(topic, p, topicLength);
          p += topicLength;

          // Read the GUID
          guid = new char(16);
          memcpy(guid, p, 16);
          p += 16;

          // Read the address length
          memcpy(&addressesLength, p, sizeof(addressesLength));
          p += sizeof(addressesLength);

          // Read the list of addresses
          addresses = new char(addressesLength);
          memcpy(addresses, p, addressesLength);

          if (this->verbose)
          {
            std::cout << "Body:" << std::endl;
            std::cout << "\tTopic length: " << topicLength << std::endl;
            std::cout << "\tTopic: " << topic << std::endl;
            std::cout << "\tGUID: " << guid << std::endl;
            std::cout << "\tAddresses length: " << addressesLength << std::endl;
            std::cout << "\tAddresses: " << addresses << std::endl;
          }

          break;

        case SUBSCRIBE:
          // Read the topic length
          memcpy(&topicLength, p, sizeof(topicLength));
          p += sizeof(topicLength);

          // Read the topic
          topic = new char(topicLength);
          memcpy(topic, p, topicLength);
          p += topicLength;

          if (this->verbose)
          {
            std::cout << "Body:" << std::endl;
            std::cout << "\tTopic length: " << topicLength << std::endl;
            std::cout << "\tTopic: " << topic << std::endl;
          }

          // Check if I advertise the topic requested

          it = std::find(topicsAdvertised.begin(), topicsAdvertised.end(),
                         topic);
          if (it != topicsAdvertised.end())
          {
            // Send to the broadcast socket an ADVERTISE message
            this->SendAdvertiseMsg(*it);
          }

          break;

        default:
          std::cerr << "DT: Unknown operation code [" << opCode << "]\n";
          break;
      }

      // Get Body length
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendAdvertiseMsg(const std::string &_topic)
    {
      // Header fields
      uint8_t opCode;
      uint16_t bodyLength;

      // Body fields
      uint16_t topicLength;
      boost::uuids::uuid guid;
      uint16_t addressesLength;
      const char *addresses;

      if (this->verbose)
      {
        std::cout << "DT: Sending ADVERTISE message" << std::endl;
      }

      // Header
      //   Fill the operation code
      opCode = ADVERTISE;

      //   The body length will be filled later

      // Body
      //   Fill the topic length
      topicLength = _topic.length();

      //   topic comes as an argument

      //   Fill the GUID
      guid = boost::uuids::random_generator()();

      //   Fill the addresses length
      std::string tcpAddress = "tcp://" + this->hostAddress +":" +
          boost::lexical_cast<std::string>(this->broadcastPort);
      std::string inprocAddress = "inproc://" + _topic;
      std::string allAddresses = tcpAddress + " " + inprocAddress;
      addressesLength = allAddresses.size();

      //   Fill the addresses
      addresses = allAddresses.data();

      // Prepare the data to send
      bodyLength = sizeof(topicLength) + topicLength + sizeof(guid) +
                   sizeof(addressesLength) + addressesLength;

      char *data = new char[sizeof(opCode) + bodyLength];
      char *p = data;
      memcpy(p, &opCode, sizeof(opCode));
      p += sizeof(opCode);
      memcpy(p, &bodyLength, sizeof(bodyLength));
      p += sizeof(bodyLength);
      memcpy(p, &topicLength, sizeof(topicLength));
      p += sizeof(topicLength);
      memcpy(p, &_topic, topicLength);
      p += topicLength;
      memcpy(p, &guid, sizeof(guid));
      p += sizeof(guid);
      memcpy(p, &addressesLength, sizeof(addressesLength));
      p += sizeof(addressesLength);
      memcpy(p, &addresses, addressesLength);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(data, strlen(data),
        this->broadcastAddress, this->broadcastPort);
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendSubscriptionMsg(const std::string &_topic)
    {
      // Header fields
      uint8_t opCode;
      uint16_t bodyLength;

      // Body fields
      uint16_t topicLength;

      if (this->verbose)
      {
        std::cout << "DT: Sending SUBSCRIPTION message" << std::endl;
      }

      // Header
      //   Fill the operation code
      opCode = SUBSCRIBE;

      //   The body length will be filled later

      // Body
      //   Fill the topic length
      topicLength = _topic.length();

      //   topic comes as an argument

      // Prepare the data to send
      bodyLength = sizeof(topicLength) + topicLength;

      char *data = new char[sizeof(opCode) + bodyLength];
      char *p = data;
      memcpy(p, &opCode, sizeof(opCode));
      p += sizeof(opCode);
      memcpy(p, &bodyLength, sizeof(bodyLength));
      p += sizeof(bodyLength);
      memcpy(p, &topicLength, sizeof(topicLength));
      p += sizeof(topicLength);
      memcpy(p, &_topic, topicLength);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(data, strlen(data),
        this->broadcastAddress, this->broadcastPort);
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service.
    /// \param[in] _topic Topic to be advertised.
    int advertise(const std::string &_topic)
    {
      assert(_topic != "");

      this->topicsAdvertised.push_back(_topic);
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a service (topic) registering a callback.
    /// \param[in] _topic Topic to be published.
    /// \param[in] _data Data to publish.
    int publish(const std::string &_topic, const std::string &_data)
    {
      assert(_topic != "");

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a service (topic) registering a callback.
    /// \param[in] _topic Topic to be subscribed.
    /// \param[in] _fp Pointer to the callback function.
    int subscribe(const std::string &_topic,
                  void(*_fp)(const std::string &, const std::string &))
    {
      assert(_topic != "");

      // Discover the list of nodes that advertise _topic

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Connect to the master.
    void connect_to_master()
    {
      /*if (this->client)
        delete this->client;

      this->client = new zmq::socket_t(*this->context, ZMQ_DEALER);
      int linger = 0;
      this->client->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
      this->identity = s_set_id(*this->client);
      this->client->connect(this->master.c_str());
      if (this->verbose)
        s_console ("I: connecting to master at %s...", this->master.c_str());*/
    }

  private:
    std::string master;
    zmq::context_t *context;
    zmq::socket_t *publisher;   //  Socket to send topic updates
    zmq::socket_t *subscriber;  //  Socket to receive topic updates
    int verbose;                //  Print activity to stdout
    int timeout;                //  Request timeout
    std::string identity;
    std::string hostAddress;

    // Callbacks for the subscribed topics
    typedef boost::function<void (const std::string &,
                                  const std::string &)> Callback;
    typedef std::map<std::string, Callback> Callback_M;
    Callback_M callbacksSub;
    boost::mutex mutexSub;

    // List of advertised topics
    std::vector<std::string> topicsAdvertised;

    // Thread in charge of the discovery
    boost::thread discoveryThread;

    // UDP broadcast thread
    std::string broadcastAddress;
    int broadcastPort;
    UDPSocket *broadcastSocket;
};

#endif
