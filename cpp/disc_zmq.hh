#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
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
      this->verbose = _verbose;
      this->timeout = 250;           // msecs

      // ToDo Read this from getenv
      this->broadcastAddress = "255.255.255.255";
      this->broadcastPort = 11312;
      this->broadcastSocket = new UDPSocket(this->broadcastPort);
      this->hostAddress = this->DetermineHost();

      // Create the GUID
      this->guid = boost::uuids::random_generator()();
      this->guidStr = boost::lexical_cast<std::string>(this->guid);

      // 0MQ
      this->context = new zmq::context_t(1);
      this->publisher = new zmq::socket_t(*this->context, ZMQ_PUB);
      this->subscriber = new zmq::socket_t(*this->context, ZMQ_SUB);
      std::string ep = "tcp://" + this->hostAddress + ":*";
      this->publisher->bind(ep.c_str());
      char bindEndPoint[1024];
      size_t size = sizeof(bindEndPoint);
      this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
      this->tcpEndpoint = bindEndPoint;

      if (this->verbose)
      {
        std::cout << "Current host address: " << this->hostAddress << std::endl;
        std::cout << "Bind at: [" << this->tcpEndpoint << "]" << std::endl;
      }
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

      this->topicsAdv.clear();
      this->addressesConnected.clear();

      Topics_M::iterator it;
      for (it = this->topicsInfo.begin(); it != this->topicsInfo.end(); ++it)
        it->second.clear();
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
     try {
        char recvString[MAXRCVSTRING];     // Buffer for data
        std::string sourceAddress;         // Address of datagram source
        unsigned short sourcePort;         // Port of datagram source
        int bytesRcvd = this->broadcastSocket->recvFrom(recvString,
          MAXRCVSTRING, sourceAddress, sourcePort);

        if (this->verbose)
        {
          cout << "\nReceived discovery update from " << sourceAddress <<
                  ": " << sourcePort << " (" << bytesRcvd << " bytes)" << endl;
        }

        if (this->DispatchDiscoveryMsg(recvString) != 0)
          std::cerr << "Something went wrong parsing a discovery message\n";

      } catch (SocketException &e) {
        cerr << e.what() << endl;
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Thread in charge of receiving the topic updates.
    void RecvTopicUpdates()
    {
      zmsg *msg = new zmsg (*this->subscriber);
      if (this->verbose)
      {
        std::cout << "\nReceived topic update" << std::endl;
        msg->dump();
      }
      // Read the DATA message
      std::string topic = std::string((char*)msg->pop_front().c_str());
      msg->pop_front(); // Sender
      std::string data = std::string((char*)msg->pop_front().c_str());

      if (this->topicsSub.find(topic) != this->topicsSub.end())
      {
        // Execute the callback registered
        Callback cb = this->topicsSub[topic];
        cb(topic, data);
      }
      else
        std::cerr << "I Don't have a callback for topic [" << topic << "]\n";
    }

    //  ---------------------------------------------------------------------
    /// \brief Receive the next message.
    void SpinOnce()
    {
      //  Poll socket for a reply, with timeout
      zmq::pollitem_t items [] = {
        { *this->subscriber, 0, ZMQ_POLLIN, 0 },
        { 0, this->broadcastSocket->sockDesc, ZMQ_POLLIN, 0 }
      };
      zmq::poll(&items[0], 2, this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
      {
        this->RecvTopicUpdates();
      }
      else if (items [1].revents & ZMQ_POLLIN)
      {
        this->RecvDiscoveryUpdates();
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Receive messages forever.
    void Spin()
    {
      while (true)
      {
        this->SpinOnce();
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
      boost::uuids::uuid otherGuid;
      uint16_t addressesLength;
      char *addresses;

      std::vector<std::string>::iterator it;
      vector<string> advTopicsV;
      vector<string> addressesV;
      std::string receivedGuid;

      // Read the operation code
      p = _msg;
      memcpy(&opCode, p, sizeof(opCode));
      p += sizeof(opCode);

      // Read the body length
      memcpy(&bodyLength, p, sizeof(bodyLength));
      p += sizeof(bodyLength);

      if (this->verbose)
      {
        std::cout << "\t--------------------------------------" << std::endl;
        std::cout << "\tHeader:" << std::endl;
        std::cout << "\t\tOp code: " << msgTypesStr[opCode] << std::endl;
        std::cout << "\t\tBody length: " << bodyLength << std::endl;
      }

      switch (opCode)
      {
        case ADVERTISE:
          // Read the topic length
          memcpy(&topicLength, p, sizeof(topicLength));
          p += sizeof(topicLength);

          // Read the topic
          topic = new char[topicLength + 1];
          memcpy(topic, p, topicLength);
          topic[topicLength] = '\0';
          p += topicLength;

          // Read the GUID
          memcpy(&otherGuid, p, otherGuid.size());
          p += otherGuid.size();
          receivedGuid = boost::lexical_cast<std::string>(otherGuid);

          // Read the address length
          memcpy(&addressesLength, p, sizeof(addressesLength));
          p += sizeof(addressesLength);

          // Read the list of addresses
          addresses = new char[addressesLength + 1];
          memcpy(addresses, p, addressesLength);
          addresses[addressesLength] = '\0';

          if (this->verbose)
          {
            std::cout << "\tBody:" << std::endl;
            std::cout << "\t\tTopic length: " << topicLength << std::endl;
            std::cout << "\t\tTopic: [" << topic << "]" << std::endl;
            std::cout << "\t\tGUID: " << receivedGuid << std::endl;
            std::cout << "\t\tAddresses length: " << addressesLength << "\n";
            std::cout << "\t\tAddresses: " << addresses << std::endl;
          }

          // Split the list of addresses
          boost::split(addressesV, addresses, boost::is_any_of(" "));
          assert(addressesV.size() > 0);

          // Split the list of advertised topics
          boost::split(advTopicsV, topic, boost::is_any_of(" "));
          assert(advTopicsV.size() > 0);

          // Update the hash of topics/addresses
          for (size_t i = 0; i < advTopicsV.size(); ++i)
          {
            std::string advTopic = advTopicsV[i];

            // Replace the list of addresses associated to the topic
            this->topicsInfo[advTopic].clear();
            for (size_t j = 0; j < addressesV.size(); ++j)
            {
              this->topicsInfo[advTopic].push_back(addressesV[j]);
            }

            // Check if we are interested in this topic
            if (this->topicsSub.find(advTopic) != this->topicsSub.end())
            {
              for (size_t j = 0; j < addressesV.size(); ++j)
              {
                std::string address;
                address = this->topicsInfo[advTopic].at(j);
                try
                {
                  // If we're already connected, don't do it again
                  std::vector<std::string>::iterator it;
                  it = std::find(addressesConnected.begin(),
                    addressesConnected.end(), address);
                  if (it != addressesConnected.end())
                    continue;

                  // Connect to an inproc address only if GUID matches
                  if (boost::starts_with(address, "inproc://") &&
                      this->guidStr.compare(receivedGuid) != 0)
                    continue;

                  // Do not connect to a non-inproc if you can choose an inproc
                  if (!boost::starts_with(address, "inproc://") &&
                      this->guidStr.compare(receivedGuid) == 0)
                    continue;

                  const char *filter = advTopic.c_str();
                  this->subscriber->setsockopt(ZMQ_SUBSCRIBE, filter,
                                               strlen(filter));
                  this->subscriber->connect(address.c_str());
                  addressesConnected.push_back(address);
                  if (this->verbose)
                    std::cout << "\t* Connected to [" << address << "]\n";
                }
                catch(const zmq::error_t& ex)
                {
                  std::cout << "Error connecting [" << zmq_errno() << "]\n";
                }
              }
            }
          }

          break;

        case SUBSCRIBE:
          // Read the topic length
          memcpy(&topicLength, p, sizeof(topicLength));
          p += sizeof(topicLength);

          // Read the topic
          topic = new char[topicLength + 1];
          topic[topicLength] = '\0';
          memcpy(topic, p, topicLength);

          if (this->verbose)
          {
            std::cout << "\tBody:" << std::endl;
            std::cout << "\t\tTopic length: " << topicLength << std::endl;
            std::cout << "\t\tTopic: [" << topic << "]" << std::endl;
          }

          // Check if I advertise the topic requested
          it = std::find(this->topicsAdv.begin(), this->topicsAdv.end(), topic);
          if (it != this->topicsAdv.end())
          {
            // Send to the broadcast socket an ADVERTISE message
            this->SendAdvertiseMsg(*it);
          }

          break;

        default:
          std::cerr << "Unknown op code [" << opCode << "] dispatching msg\n";
          break;
      }

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendAdvertiseMsg(const std::string &_topic)
    {
      if (this->verbose)
      {
        std::cout << "\t* Sending ADVERTISE message [" << _topic << "]\n";
      }

      // Header
      //   Fill the operation code
      uint8_t opCode = ADVERTISE;

      //   The body length will be filled later

      // Body
      //   Fill the topic length
      uint16_t topicLength = _topic.size();

      //   topic comes as an argument

      //   Fill the GUID
      boost::uuids::uuid guid = this->guid;

      //   Fill the addresses length
      std::string inprocAddress = "inproc://" + _topic;
      std::string allAddresses = this->tcpEndpoint + " " + inprocAddress;
      uint16_t addressesLength = allAddresses.size();

      // Prepare the data to send
      uint16_t bodyLength = sizeof(topicLength) + topicLength + sizeof(guid) +
                   sizeof(addressesLength) + addressesLength;

      int dataLength = sizeof(opCode) + sizeof(bodyLength) + bodyLength;
      char *data = new char[dataLength];
      char *p = data;
      memcpy(p, &opCode, sizeof(opCode));
      p += sizeof(opCode);
      memcpy(p, &bodyLength, sizeof(bodyLength));
      p += sizeof(bodyLength);
      memcpy(p, &topicLength, sizeof(topicLength));
      p += sizeof(topicLength);
      memcpy(p, _topic.data(), topicLength);
      p += topicLength;
      memcpy(p, &this->guid, this->guid.size());
      p += this->guid.size();
      memcpy(p, &addressesLength, sizeof(addressesLength));
      p += sizeof(addressesLength);
      memcpy(p, allAddresses.data(), addressesLength);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(data, dataLength,
        this->broadcastAddress, this->broadcastPort);
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendSubscribeMsg(const std::string &_topic)
    {
      if (this->verbose)
      {
        std::cout << "\t * Sending SUBSCRIPTION message" << std::endl;
      }

      // Header
      //   Fill the operation code
      uint8_t opCode = SUBSCRIBE;

      //   The body length will be filled later

      // Body
      //   Fill the topic length
      uint16_t topicLength = _topic.size();

      //   topic comes as an argument

      // Prepare the data to send
      uint16_t bodyLength = sizeof(topicLength) + topicLength;

      int dataLength = sizeof(opCode) + sizeof(bodyLength) + bodyLength;
      char *data = new char[dataLength];
      char *p = data;
      memcpy(p, &opCode, sizeof(opCode));
      p += sizeof(opCode);
      memcpy(p, &bodyLength, sizeof(bodyLength));
      p += sizeof(bodyLength);
      memcpy(p, &topicLength, sizeof(topicLength));
      p += sizeof(topicLength);
      memcpy(p, _topic.data(), topicLength);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(data, dataLength,
          this->broadcastAddress, this->broadcastPort);
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service.
    /// \param[in] _topic Topic to be advertised.
    int advertise(const std::string &_topic)
    {
      assert(_topic != "");

      // Only bind to the topic and add to the advertised topic list, once
      std::vector<std::string>::iterator it;
      it = std::find(this->topicsAdv.begin(), this->topicsAdv.end(), _topic);
      if (it == this->topicsAdv.end())
      {
        this->topicsAdv.push_back(_topic);

        // Bind using the inproc address
        std::string inprocEP = "inproc://" + _topic;
        this->publisher->bind(inprocEP.c_str());
        if (this->verbose)
        {
          std::cout << "\nAdvertise(" << _topic << ")\n";
          std::cout << "\t* Bind at: [" << inprocEP << "]\n";
        }
      }

      this->SendAdvertiseMsg(_topic);
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a service (topic) registering a callback.
    /// \param[in] _topic Topic to be published.
    /// \param[in] _data Data to publish.
    int publish(const std::string &_topic, const std::string &_data)
    {
      assert(_topic != "");

      zmsg msg;
      std::string sender = this->tcpEndpoint + " inproc://" + _topic;
      msg.push_back((char*)_topic.c_str());
      msg.push_back((char*)sender.c_str());
      msg.push_back((char*)_data.c_str());

      if (this->verbose)
      {
        std::cout << "\nPublish(" << _topic << ")" << std::endl;
        msg.dump();
      }
      msg.send (*this->publisher);

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
      if (this->verbose)
        std::cout << "\nSubscribe (" << _topic << ")\n";

      // Register our interest on the topic
      // The last subscribe call replaces previous subscriptions. If this is
      // a problem, we have to store a list of callbacks.
      this->topicsSub[_topic] = _fp;

      // Discover the list of nodes that publish on the topic
      this->SendSubscribeMsg(_topic);
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service call registering a callback.
    /// \param[in] _topic Topic to be advertised.
    /// \param[in] _fp Pointer to the callback function.
    int srv_advertise(const std::string &_topic,
      int(*_fp)(const std::string &, const std::string &, std::string &))
    {
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Request a new service to another component using a blocking call.
    /// \param[in] _topic Topic requested.
    /// \param[in] _data Data of the request.
    /// \param[out] _response Response of the request.
    int srv_request(const std::string &_topic, const std::string &_data,
      std::string &_response)
    {
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Request a new service to another component using a non-blocking
    /// call.
    /// \param[in] _topic Topic requested.
    /// \param[in] _data Data of the request.
    /// \param[in] _fp Pointer to the callback function.
    int srv_request_async(const std::string &_topic, const std::string &_data,
      void(*_fp)(const std::string &_topic, int rc, const std::string &_rep))
    {
      return 0;
    }

  private:
    // Master address
    std::string master;

    // Print activity to stdout
    int verbose;

    // Hash with the topic/addresses information
    typedef std::vector<std::string> Topics_L;
    typedef std::map<std::string, Topics_L> Topics_M;
    Topics_M topicsInfo;

    // Advertised topics
    std::vector<std::string> topicsAdv;

    // Subscribed topics
    typedef boost::function<void (const std::string &,
                                  const std::string &)> Callback;
    typedef std::map<std::string, Callback> Callback_M;
    Callback_M topicsSub;

    // Connected addresses
    std::vector<std::string> addressesConnected;

    // UDP broadcast thread
    std::string hostAddress;
    std::string broadcastAddress;
    int broadcastPort;
    UDPSocket *broadcastSocket;

    // 0MQ Sockets
    zmq::context_t *context;
    zmq::socket_t *publisher;   //  Socket to send topic updates
    zmq::socket_t *subscriber;  //  Socket to receive topic updates
    std::string tcpEndpoint;
    std::string identity;
    int timeout;                //  Request timeout

    // GUID
    boost::uuids::uuid guid;
    std::string guidStr;
};

#endif
