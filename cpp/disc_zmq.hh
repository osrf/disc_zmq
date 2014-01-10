#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <map>
#include <string>

#include "packet.hh"
#include "sockets/socket.hh"
#include "zmq/zmq.hpp"
#include "zmq/zmsg.hpp"

const int MAXRCVSTRING = 65536; // Longest string to receive

class Node
{
  public:

    //  ---------------------------------------------------------------------
    /// \brief Constructor.
    /// \param[in] _master End point with the master's endpoint.
    /// \param[in] _verbose true for enabling verbose mode.
    Node (std::string _master, bool _verbose)
    {
      char bindEndPoint[1024];

      // Initialize random seed
      srand (time(NULL));

      // Required 0mq minimum version
      s_version_assert(2, 1);

      this->master = _master;
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
      this->srvRequester = new zmq::socket_t(*this->context, ZMQ_DEALER);
      this->srvReplier = new zmq::socket_t(*this->context, ZMQ_DEALER);
      std::string ep = "tcp://" + this->hostAddress + ":*";
      this->publisher->bind(ep.c_str());
      size_t size = sizeof(bindEndPoint);
      this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
      this->tcpEndpoint = bindEndPoint;
      this->addresses.push_back(this->tcpEndpoint);
      this->srvReplier->bind(ep.c_str());
      this->srvReplier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
      this->srvAddresses.push_back(bindEndPoint);

      if (this->verbose)
      {
        std::cout << "Current host address: " << this->hostAddress << std::endl;
        std::cout << "Bind at: [" << this->tcpEndpoint << "] for pub/sub\n";
        std::cout << "Bind at: [" << bindEndPoint << "] for req/rep\n";
        std::cout << "GUID: " << this->guidStr << std::endl;
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
      if (this->srvRequester)
        delete this->srvRequester;
      if (this->srvReplier)
        delete this->srvReplier;
      if (this->context)
        delete this->context;

      this->srvAddresses.clear();
      this->topicsAdv.clear();
      this->addressesConnected.clear();

      Topics_M::iterator it;
      for (it = this->topicsInfo.begin(); it != this->topicsInfo.end(); ++it)
        it->second.clear();
    }

    //  ---------------------------------------------------------------------
    /// \brief Determine IP or hostname.
    std::string DetermineHost()
    {
      char *ip_env;
      // First, did the user set DZMQ_IP?
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
    /// \brief Method in charge of receiving the discovery updates.
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
    /// \brief Method in charge of receiving the topic updates.
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
    /// \brief Method in charge of receiving the topic updates.
    void RecvSrvRequest()
    {
      zmsg *msg = new zmsg (*this->srvReplier);
      if (this->verbose)
      {
        std::cout << "\nReceived service request" << std::endl;
        msg->dump();
      }
      // Read the REQ message
      std::string topic = std::string((char*)msg->pop_front().c_str());
      msg->pop_front(); // Sender
      std::string data = std::string((char*)msg->pop_front().c_str());

      if (this->srvsAdv.find(topic) != this->srvsAdv.end())
      {
        // Execute the callback registered
        RepCallback cb = this->srvsAdv[topic];

        std::string response;
        int rc = cb(topic, data, response);

        // Send the service call response
        std::string sender = this->tcpEndpoint + " inproc://" + topic;
        zmsg reply;
        reply.push_back((char*)topic.c_str());
        reply.push_back((char*)sender.c_str());
        reply.push_back((char*)response.c_str());
        // Todo: include the return code

        if (this->verbose)
        {
          std::cout << "\nResponse (" << topic << ")" << std::endl;
          reply.dump();
        }
        reply.send (*this->srvReplier);
      }
      else
      {
        std::cerr << "Received a service call that I'm not advertising ("
                  << topic << ")\n";
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Receive the next message.
    void SpinOnce()
    {
      //  Poll socket for a reply, with timeout
      zmq::pollitem_t items [] = {
        { *this->subscriber, 0, ZMQ_POLLIN, 0 },
        { *this->srvReplier, 0, ZMQ_POLLIN, 0 },
        { 0, this->broadcastSocket->sockDesc, ZMQ_POLLIN, 0 }
      };
      zmq::poll(&items[0], 3, this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
      {
        this->RecvTopicUpdates();
      }
      else if (items [1].revents & ZMQ_POLLIN)
      {
        this->RecvSrvRequest();
      }
      else if (items [2].revents & ZMQ_POLLIN)
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
      Header header;
      AdvMsg advMsg;
      uint16_t addressLength;
      char *address;
      std::vector<std::string>::iterator it;
      char *p = _msg;

      header.Unpack(_msg);
      p += header.GetHeaderLength();

      std::string advTopic = header.GetTopic();
      std::string otherGuidStr =
        boost::lexical_cast<std::string>(header.GetGuid());

      if (this->verbose)
      {
        std::cout << "\t--------------------------------------" << std::endl;
        std::cout << "\tHeader:" << std::endl;
        std::cout << "\t\tVersion: " << header.GetVersion() << std::endl;
        std::cout << "\t\tGUID: " << header.GetGuid() << std::endl;
        std::cout << "\t\tTopic length: " << header.GetTopicLength() << std::endl;
        std::cout << "\t\tTopic: [" << header.GetTopic() << "]" << std::endl;
        std::cout << "\t\tType: " << msgTypesStr[header.GetType()] << std::endl;
        std::cout << "\t\tFlags: " << header.GetFlags() << std::endl;
      }

      switch (header.GetType())
      {
        case ADV:
          advMsg.UnpackBody(p);
          address = strdup(advMsg.GetAddress().c_str());

          if (this->verbose)
          {
            std::cout << "\tBody:" << std::endl;
            std::cout << "\t\tAddress length: " << advMsg.GetAddressLength() << "\n";
            std::cout << "\t\tAddress: " << advMsg.GetAddress() << std::endl;
          }

          // If I already have the topic/address registered, just skip it

          if (this->topicsInfo.find(advTopic) != this->topicsInfo.end())
          {
            std::vector<std::string>::iterator it;
            it = find(this->topicsInfo[advTopic].begin(),
                      this->topicsInfo[advTopic].end(), address);

            if (it != this->topicsInfo[advTopic].end())
              break;
          }

          // Update the hash of topics/addresses
          this->topicsInfo[advTopic].push_back(address);

          // Check if we are interested in this topic
          if (this->topicsSub.find(advTopic) != this->topicsSub.end())
          {
            try
            {
              // If we're already connected, don't do it again
              std::vector<std::string>::iterator it;
              it = std::find(this->addressesConnected.begin(),
                             this->addressesConnected.end(), address);
              if (it != this->addressesConnected.end())
                break;

              // Connect to an inproc address only if GUID matches
              if (boost::starts_with(address, "inproc://") &&
                  this->guidStr.compare(otherGuidStr) != 0)
                break;

              // Do not connect to a non-inproc if you can choose an inproc
              if (!boost::starts_with(address, "inproc://") &&
                  this->guidStr.compare(otherGuidStr) == 0)
                break;

              const char *filter = advTopic.data();
              this->subscriber->setsockopt(ZMQ_SUBSCRIBE, filter,
                                           strlen(filter));
              this->subscriber->connect(address);
              this->addressesConnected.push_back(address);
              if (this->verbose)
                std::cout << "\t* Connected to [" << address << "]\n";
            }
            catch (const zmq::error_t& ex)
            {
              std::cout << "Error connecting [" << zmq_errno() << "]\n";
            }
          }

          break;

        case ADV_SVC:
          // Read the address length
          memcpy(&addressLength, p, sizeof(addressLength));
          p += sizeof(addressLength);

          // Read the address
          address = new char[addressLength + 1];
          memcpy(address, p, addressLength);
          address[addressLength] = '\0';

          if (this->verbose)
          {
            std::cout << "\tBody:" << std::endl;
            std::cout << "\t\tAddress length: " << addressLength << "\n";
            std::cout << "\t\tAddress: " << address << std::endl;
          }

          // If I already have the topic/address registered, just skip it
          if (this->srvsInfo.find(advTopic) != this->srvsInfo.end())
          {
            std::vector<std::string>::iterator it;
            it = find(this->srvsInfo[advTopic].begin(),
                      this->srvsInfo[advTopic].end(), address);

            if (it != this->srvsInfo[advTopic].end())
              break;
          }

          // Update the hash of topics/addresses
          this->srvsInfo[advTopic].push_back(address);

          // Check if we are interested in this service call
          if (this->srvsReq.find(advTopic) != this->srvsReq.end())
          {
            try
            {
              // If we're already connected, don't do it again
              std::vector<std::string>::iterator it;
              it = std::find(this->srvsAddressesConnected.begin(),
                             this->srvsAddressesConnected.end(), address);
              if (it != this->srvsAddressesConnected.end())
                break;

              // Connect to an inproc address only if GUID matches
              if (boost::starts_with(address, "inproc://") &&
                  this->guidStr.compare(otherGuidStr) != 0)
                break;

              // Do not connect to a non-inproc if you can choose an inproc
              /*if (!boost::starts_with(address, "inproc://") &&
                  this->guidStr.compare(otherGuidStr) == 0)
                break;*/

              this->srvRequester->connect(address);
              this->srvsAddressesConnected.push_back(address);
              if (this->verbose)
                std::cout << "\t* Connected to [" << address << "]\n";
            }
            catch (const zmq::error_t& ex)
            {
              std::cout << "Error connecting [" << zmq_errno() << "]\n";
            }
          }

          break;

        case SUB:

          // Check if I advertise the topic requested
          it = std::find(this->topicsAdv.begin(), this->topicsAdv.end(),
                         advTopic);
          if (it != this->topicsAdv.end())
          {
            // Send to the broadcast socket an ADVERTISE message
            std::vector<std::string>::iterator it2;
            for (it2 = this->addresses.begin(); it2 != this->addresses.end();
                 ++it2)
              this->SendAdvertiseMsg(ADV, advTopic, *it2);
          }

          break;

        case SUB_SVC:

          // Check if I advertise the service call requested
          if (this->srvsAdv.find(advTopic) != this->srvsAdv.end());
          {
            // Send to the broadcast socket an ADV_SVC message
            std::vector<std::string>::iterator it2;
            for (it2 = this->srvAddresses.begin();
                 it2 != this->srvAddresses.end(); ++it2)
            {
              this->SendAdvertiseMsg(ADV_SVC, advTopic, *it2);
            }
          }

          break;

        default:
          std::cerr << "Unknown msg type [" << header.GetType() << "] dispatching msg\n";
          break;
      }

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendAdvertiseMsg(uint8_t _type, const std::string &_topic,
                         const std::string &_address)
    {
      if (this->verbose)
      {
        std::cout << "\t* Sending ADV msg [" << _topic << "][" << _address
                  << "]" << std::endl;
      }

      Header header(TRNSP_VERSION, this->guid, _topic.size(), _topic, _type, 0);
      AdvMsg advMsg(header, _address.size(), _address);

      char *buffer = new char[advMsg.GetMsgLength()];
      advMsg.Pack(buffer);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(buffer, advMsg.GetMsgLength(),
        this->broadcastAddress, this->broadcastPort);

      delete[] buffer;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    int SendSubscribeMsg(uint8_t _type, const std::string &_topic)
    {
      if (this->verbose)
      {
        std::cout << "\t * Sending SUBSCRIPTION message" << std::endl;
      }

      Header header(TRNSP_VERSION, this->guid, _topic.size(), _topic, _type, 0);

      char *buffer = new char[header.GetHeaderLength()];
      header.Pack(buffer);

      // Send the data through the UDP broadcast socket
      this->broadcastSocket->sendTo(buffer, header.GetHeaderLength(),
        this->broadcastAddress, this->broadcastPort);

      delete[] buffer;
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
        this->addresses.push_back(inprocEP);

        if (this->verbose)
        {
          std::cout << "\nAdvertise(" << _topic << ")\n";
          std::cout << "\t* Bind at: [" << inprocEP << "]\n";
        }
      }

      for (it = this->addresses.begin(); it != this->addresses.end(); ++it)
        this->SendAdvertiseMsg(ADV, _topic, *it);

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Publish data.
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
    /// \brief Subscribe to a topic registering a callback.
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
      this->SendSubscribeMsg(SUB, _topic);
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service call registering a callback.
    /// \param[in] _topic Topic to be advertised.
    /// \param[in] _fp Pointer to the callback function.
    int srv_advertise(const std::string &_topic,
      int(*_fp)(const std::string &, const std::string &, std::string &))
    {
      assert(_topic != "");

      // Only advertise the same service call once
      if (this->srvsAdv.find(_topic) == this->srvsAdv.end());
      {
        this->srvsAdv[_topic] = _fp;

        if (this->verbose)
        {
          std::cout << "\nAdvertise srv call(" << _topic << ")\n";
        }
      }

      std::vector<std::string>::iterator it;
      for (it = this->srvAddresses.begin(); it != this->srvAddresses.end();
           ++it)
        this->SendAdvertiseMsg(ADV_SVC, _topic, *it);

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
      assert(_topic != "");

      srvsReq[_topic] = NULL;

      while (this->srvsInfo.find(_topic) == this->srvsInfo.end())
      {
        this->SendSubscribeMsg(SUB_SVC, _topic);
        this->SpinOnce();
      }

      // Send the request
      std::string sender = this->tcpEndpoint + " inproc://" + _topic;
      zmsg msg;
      msg.push_back((char*)_topic.c_str());
      msg.push_back((char*)sender.c_str());
      msg.push_back((char*)_data.c_str());

      if (this->verbose)
      {
        std::cout << "\nRequest (" << _topic << ")" << std::endl;
        std::cout << "\t* Found node advertising the service call at "
                  << this->srvsInfo[_topic].at(0) << std::endl;
        msg.dump();
      }
      msg.send (*this->srvRequester);

      //  Poll socket for a reply, with timeout
      zmq::pollitem_t items [] = { { *this->srvRequester, 0, ZMQ_POLLIN, 0 } };
      zmq::poll(items, 1, this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
      {
        zmsg *reply = new zmsg (*this->srvRequester);

        std::string topic = std::string((char*)reply->pop_front().c_str());
        std::string sender = std::string((char*)reply->pop_front().c_str());
        _response = std::string((char*)reply->pop_front().c_str());

        return 0;
      }

      return -1;
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

    // Hash with the topic/addresses information for pub/sub
    typedef std::vector<std::string> Topics_L;
    typedef std::map<std::string, Topics_L> Topics_M;
    Topics_M topicsInfo;

    // Hash with the topic/addresses information for service calls
    Topics_M srvsInfo;

    // Advertised topics
    std::vector<std::string> topicsAdv;

    // Subscribed topics
    typedef boost::function<void (const std::string &,
                                  const std::string &)> Callback;
    typedef std::map<std::string, Callback> Callback_M;
    Callback_M topicsSub;

    // Advertised service calls
    typedef boost::function<void (const std::string &, int,
                                  const std::string &)> ReqCallback;
    typedef boost::function<int (const std::string &, const std::string &,
                                 std::string &)> RepCallback;
    typedef std::map<std::string, ReqCallback> ReqCallback_M;
    typedef std::map<std::string, RepCallback> RepCallback_M;
    RepCallback_M srvsAdv;

    // Requested service calls
    ReqCallback_M srvsReq;

    // My pub/sub address
    std::vector<std::string> addresses;

    // My req/rep address
    std::vector<std::string> srvAddresses;

    // Pub/sub connected addresses
    std::vector<std::string> addressesConnected;

    // Srv calls connected addresses
    std::vector<std::string> srvsAddressesConnected;

    // UDP broadcast thread
    std::string hostAddress;
    std::string broadcastAddress;
    int broadcastPort;
    UDPSocket *broadcastSocket;

    // 0MQ Sockets
    zmq::context_t *context;
    zmq::socket_t *publisher;     //  Socket to send topic updates
    zmq::socket_t *subscriber;    //  Socket to receive topic updates
    zmq::socket_t *srvRequester;  //  Socket to send service call requests
    zmq::socket_t *srvReplier;    //  Socket to receive service call requests
    std::string tcpEndpoint;
    int timeout;                  //  Request timeout

    // GUID
    boost::uuids::uuid guid;
    std::string guidStr;
};

#endif
