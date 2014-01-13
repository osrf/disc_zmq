#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <string>

#include "netUtils.hh"
#include "packet.hh"
#include "sockets/socket.hh"
#include "topicsInfo.hh"
#include "zmq/zmq.hpp"
#include "zmq/zmsg.hpp"

const int MaxRcvStr = 65536; // Longest string to receive
const std::string InprocAddr = "inproc://local";

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

      // ToDo Read this from getenv or command line arguments
      this->bcastAddr = "255.255.255.255";
      this->bcastPort = 11312;
      this->bcastSock = new UDPSocket(this->bcastPort);
      this->hostAddr = DetermineHost();

      // Create the GUID
      this->guid = boost::uuids::random_generator()();
      this->guidStr = boost::lexical_cast<std::string>(this->guid);

      // 0MQ
      try
      {
        this->context = new zmq::context_t(1);
        this->publisher = new zmq::socket_t(*this->context, ZMQ_PUB);
        this->subscriber = new zmq::socket_t(*this->context, ZMQ_SUB);
        this->srvRequester = new zmq::socket_t(*this->context, ZMQ_DEALER);
        this->srvReplier = new zmq::socket_t(*this->context, ZMQ_DEALER);
        std::string ep = "tcp://" + this->hostAddr + ":*";
        this->publisher->bind(ep.c_str());
        size_t size = sizeof(bindEndPoint);
        this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
        this->publisher->bind(InprocAddr.c_str());
        this->tcpEndpoint = bindEndPoint;
        this->myAddresses.push_back(this->tcpEndpoint);
        this->subscriber->connect(InprocAddr.c_str());

        this->srvReplier->bind(ep.c_str());
        this->srvReplier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
        this->mySrvAddresses.push_back(bindEndPoint);
      }
      catch(const zmq::error_t& ze)
      {
         std::cerr << "Error: " << ze.what() << std::endl;
         this->Fini();
         exit(EXIT_FAILURE);
      }

      if (this->verbose)
      {
        std::cout << "Current host address: " << this->hostAddr << std::endl;
        std::cout << "Bind at: [" << this->tcpEndpoint << "] for pub/sub\n";
        std::cout << "Bind at: [" << InprocAddr << "] for pub/sub\n";
        std::cout << "Bind at: [" << bindEndPoint << "] for req/rep\n";
        std::cout << "GUID: " << this->guidStr << std::endl;
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Destructor.
    virtual ~Node()
    {
      this->Fini();
    }

    //  ---------------------------------------------------------------------
    /// \brief Deallocate resources.
    void Fini()
    {
      if (this->publisher) delete this->publisher;
      if (this->publisher) delete this->subscriber;
      if (this->publisher) delete this->srvRequester;
      if (this->publisher) delete this->srvReplier;
      if (this->publisher) delete this->context;

      this->myAddresses.clear();
      this->mySrvAddresses.clear();
    }

    //  ---------------------------------------------------------------------
    /// \brief Method in charge of receiving the discovery updates.
    void RecvDiscoveryUpdates()
    {
      char rcvStr[MaxRcvStr];     // Buffer for data
      std::string srcAddr;           // Address of datagram source
      unsigned short srcPort;        // Port of datagram source
      int bytes;                 // Rcvd from the UDP broadcast socket

      try {
        bytes = this->bcastSock->recvFrom(rcvStr, MaxRcvStr, srcAddr, srcPort);
      } catch (SocketException &e) {
        cerr << "Exception receiving from the UDP socket: " << e.what() << endl;
        return;
      }

      if (this->verbose)
        cout << "\nReceived discovery update from " << srcAddr <<
                ": " << srcPort << " (" << bytes << " bytes)" << endl;

      if (this->DispatchDiscoveryMsg(rcvStr) != 0)
        std::cerr << "Something went wrong parsing a discovery message\n";
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

      if (msg->parts() != 3)
      {
        std::cerr << "Unexpected topic update. Expected 3 message parts but "
                  << "received a message with " << msg->parts() << std::endl;
        return;
      }

      // Read the DATA message
      std::string topic = std::string((char*)msg->pop_front().c_str());
      msg->pop_front(); // Sender
      std::string data = std::string((char*)msg->pop_front().c_str());

      if (this->topics.Subscribed(topic))
      {
        // Execute the callback registered
        TopicsInfo::Callback cb;
        if (this->topics.GetCallback(topic, cb))
          cb(topic, data);
        else
          std::cerr << "I don't have a callback for topic [" << topic << "]\n";
      }
      else
        std::cerr << "I am not subscribed to topic [" << topic << "]\n";
    }

    //  ---------------------------------------------------------------------
    /// \brief Method in charge of receiving the service call requests.
    void RecvSrvRequest()
    {
      zmsg *msg = new zmsg (*this->srvReplier);
      if (this->verbose)
      {
        std::cout << "\nReceived service request" << std::endl;
        msg->dump();
      }

      if (msg->parts() != 3)
      {
        std::cerr << "Unexpected service request. Expected 3 message parts but "
                  << "received a message with " << msg->parts() << std::endl;
        return;
      }

      // Read the REQ message
      std::string topic = std::string((char*)msg->pop_front().c_str());
      msg->pop_front(); // Sender
      std::string data = std::string((char*)msg->pop_front().c_str());

      if (this->topicsSrvs.AdvertisedByMe(topic))
      {
        // Execute the callback registered
        TopicsInfo::RepCallback cb;
        std::string response;
        if (this->topicsSrvs.GetRepCallback(topic, cb))
          int rc = cb(topic, data, response);
        else
          std::cerr << "I don't have a REP cback for topic [" << topic << "]\n";

        // Send the service call response
        zmsg reply;
        reply.push_back((char*)topic.c_str());
        reply.push_back((char*)this->tcpEndpoint.c_str());
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
        std::cerr << "Received a svc call not advertised (" << topic << ")\n";
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
        { 0, this->bcastSock->sockDesc, ZMQ_POLLIN, 0 }
      };
      zmq::poll(&items[0], 3, this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
        this->RecvTopicUpdates();
      else if (items [1].revents & ZMQ_POLLIN)
        this->RecvSrvRequest();
      else if (items [2].revents & ZMQ_POLLIN)
        this->RecvDiscoveryUpdates();
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
    /// \return 0 when success.
    int DispatchDiscoveryMsg(char *_msg)
    {
      Header header;
      AdvMsg advMsg;
      std::string address;
      char *pBody = _msg;
      std::vector<std::string>::iterator it;

      header.Unpack(_msg);
      pBody += header.GetHeaderLength();

      std::string topic = header.GetTopic();
      std::string rcvdGuid = boost::lexical_cast<std::string>(header.GetGuid());

      if (this->verbose)
        header.Print();

      switch (header.GetType())
      {
        case ADV:
          // Read the address
          advMsg.UnpackBody(pBody);
          address = advMsg.GetAddress();

          if (this->verbose)
            advMsg.PrintBody();

          // Register the advertised address for the topic
          this->topics.AddAdvAddress(topic, address);

          // Check if we are interested in this topic
          if (this->topics.Subscribed(topic) &&
              !this->topics.Connected(topic) &&
              this->guidStr.compare(rcvdGuid) != 0)
          {
            try
            {
              this->subscriber->connect(address.c_str());
              this->topics.SetConnected(topic, true);
              if (this->verbose)
                std::cout << "\t* Connected to [" << address << "]\n";
            }
            catch (const zmq::error_t& ze)
            {
              std::cout << "Error connecting [" << ze.what() << "]\n";
            }
          }

          break;

        case ADV_SVC:
          // Read the address
          advMsg.UnpackBody(pBody);
          address = advMsg.GetAddress();

          if (this->verbose)
            advMsg.PrintBody();

          // Register the advertised address for the service call
          this->topicsSrvs.AddAdvAddress(topic, address);

          // Check if we are interested in this service call
          if (this->topicsSrvs.Requested(topic) &&
              !this->topicsSrvs.Connected(topic) &&
              this->guidStr.compare(rcvdGuid) != 0)
          {
            try
            {
              this->srvRequester->connect(address.c_str());
              this->topicsSrvs.SetConnected(topic, true);
              if (this->verbose)
                std::cout << "\t* Connected to [" << address << "]\n";
            }
            catch (const zmq::error_t& ze)
            {
              std::cout << "Error connecting [" << ze.what() << "]\n";
            }
          }

          break;

        case SUB:
          // Check if I advertise the topic requested
          if (this->topics.AdvertisedByMe(topic))
          {
            // Send to the broadcast socket an ADVERTISE message
            for (it = this->myAddresses.begin();
                 it != this->myAddresses.end(); ++it)
              this->SendAdvertiseMsg(ADV, topic, *it);
          }

          break;

        case SUB_SVC:
          // Check if I advertise the service call requested
          if (this->topicsSrvs.AdvertisedByMe(topic))
          {
            // Send to the broadcast socket an ADV_SVC message
            for (it = this->mySrvAddresses.begin();
                 it != this->mySrvAddresses.end(); ++it)
            {
              this->SendAdvertiseMsg(ADV_SVC, topic, *it);
            }
          }

          break;

        default:
          std::cerr << "Unknown message type [" << header.GetType() << "]\n";
          break;
      }

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send an ADVERTISE message to the discovery socket.
    /// \param[in] _type ADV or ADV_SVC.
    /// \param[in] _topic Topic to be advertised.
    /// \param[in] _address Address to be advertised with the topic.
    /// \return 0 when success.
    int SendAdvertiseMsg(uint8_t _type, const std::string &_topic,
                         const std::string &_address)
    {
      assert(_topic != "");

      if (this->verbose)
        std::cout << "\t* Sending ADV msg [" << _topic << "][" << _address
                  << "]" << std::endl;

      Header header(TRNSP_VERSION, this->guid, _topic.size(), _topic, _type, 0);
      AdvMsg advMsg(header, _address.size(), _address);

      char *buffer = new char[advMsg.GetMsgLength()];
      advMsg.Pack(buffer);

      // Send the data through the UDP broadcast socket
      try
      {
        this->bcastSock->sendTo(buffer, advMsg.GetMsgLength(),
          this->bcastAddr, this->bcastPort);
      } catch (SocketException &e) {
        cerr << "Exception sending an ADV msg: " << e.what() << endl;
      }

      delete[] buffer;
    }

    //  ---------------------------------------------------------------------
    /// \brief Send a SUBSCRIBE message to the discovery socket.
    /// \param[in] _type SUB or SUB_SVC.
    /// \param[in] _topic Topic name.
    /// \return 0 when success.
    int SendSubscribeMsg(uint8_t _type, const std::string &_topic)
    {
      assert(_topic != "");

      if (this->verbose)
        std::cout << "\t* Sending SUB msg [" << _topic << "]" << std::endl;

      Header header(TRNSP_VERSION, this->guid, _topic.size(), _topic, _type, 0);

      char *buffer = new char[header.GetHeaderLength()];
      header.Pack(buffer);

      // Send the data through the UDP broadcast socket
      try
      {
        this->bcastSock->sendTo(buffer, header.GetHeaderLength(),
          this->bcastAddr, this->bcastPort);
      } catch (SocketException &e) {
        cerr << "Exception sending a SUB msg: " << e.what() << endl;
      }

      delete[] buffer;
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service.
    /// \param[in] _topic Topic to be advertised.
    int advertise(const std::string &_topic)
    {
      assert(_topic != "");

      this->topics.SetAdvertisedByMe(_topic, true);

      std::vector<std::string>::iterator it;
      for (it = this->myAddresses.begin(); it != this->myAddresses.end(); ++it)
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
      std::string sender = this->tcpEndpoint;
      msg.push_back((char*)_topic.c_str());
      msg.push_back((char*)sender.c_str());
      msg.push_back((char*)_data.c_str());

      if (this->verbose)
      {
        std::cout << "\nPublish(" << _topic << ")" << std::endl;
        msg.dump();
      }
      msg.send(*this->publisher);

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a topic registering a callback.
    /// \param[in] _topic Topic to be subscribed.
    /// \param[in] _cb Pointer to the callback function.
    int subscribe(const std::string &_topic,
      void(*_cb)(const std::string &, const std::string &))
    {
      assert(_topic != "");
      if (this->verbose)
        std::cout << "\nSubscribe (" << _topic << ")\n";

      // Register our interest on the topic
      // The last subscribe call replaces previous subscriptions. If this is
      // a problem, we have to store a list of callbacks.
      this->topics.SetSubscribed(_topic, true);
      this->topics.SetCallback(_topic, _cb);

      // Add a filter for this topic
      this->subscriber->setsockopt(ZMQ_SUBSCRIBE, _topic.data(), _topic.size());

      // Discover the list of nodes that publish on the topic
      this->SendSubscribeMsg(SUB, _topic);
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service call registering a callback.
    /// \param[in] _topic Topic to be advertised.
    /// \param[in] _cb Pointer to the callback function.
    int srv_advertise(const std::string &_topic,
      int(*_cb)(const std::string &, const std::string &, std::string &))
    {
      assert(_topic != "");

      this->topicsSrvs.SetAdvertisedByMe(_topic, true);
      this->topicsSrvs.SetRepCallback(_topic, _cb);

      if (this->verbose)
          std::cout << "\nAdvertise srv call(" << _topic << ")\n";

      std::vector<std::string>::iterator it;
      for (it = this->mySrvAddresses.begin();
           it != this->mySrvAddresses.end(); ++it)
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

      this->topicsSrvs.SetRequested(_topic, true);

      while (!this->topicsSrvs.Connected(_topic))
      {
        this->SendSubscribeMsg(SUB_SVC, _topic);
        this->SpinOnce();
      }

      // Send the request
      zmsg msg;
      msg.push_back((char*)_topic.c_str());
      msg.push_back((char*)this->tcpEndpoint.c_str());
      msg.push_back((char*)_data.c_str());

      if (this->verbose)
      {
        std::cout << "\nRequest (" << _topic << ")" << std::endl;
        msg.dump();
      }
      msg.send (*this->srvRequester);

      // Poll socket for a reply, with timeout
      zmq::pollitem_t items [] = { { *this->srvRequester, 0, ZMQ_POLLIN, 0 } };
      zmq::poll(items, 1, this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
      {
        zmsg *reply = new zmsg (*this->srvRequester);

        std::string((char*)reply->pop_front().c_str());
        std::string((char*)reply->pop_front().c_str());
        _response = std::string((char*)reply->pop_front().c_str());

        return 0;
      }

      return -1;
    }

    //  ---------------------------------------------------------------------
    /// \brief Request a new service call using a non-blocking call.
    /// \param[in] _topic Topic requested.
    /// \param[in] _data Data of the request.
    /// \param[in] _cb Pointer to the callback function.
    int srv_request_async(const std::string &_topic, const std::string &_data,
      void(*_cb)(const std::string &_topic, int rc, const std::string &_rep))
    {
      assert(_topic != "");

      this->topicsSrvs.SetRequested(_topic, true);
      this->topicsSrvs.SetReqCallback(_topic, _cb);

      this->SendSubscribeMsg(SUB_SVC, _topic);

      return 0;
    }

  private:
    // Master address
    std::string master;

    // Print activity to stdout
    int verbose;

    // Topic information
    TopicsInfo topics;
    TopicsInfo topicsSrvs;

    // My pub/sub address
    std::vector<std::string> myAddresses;

    // My req/rep address
    std::vector<std::string> mySrvAddresses;

    // UDP broadcast thread
    std::string hostAddr;
    std::string bcastAddr;
    int bcastPort;
    UDPSocket *bcastSock;

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
