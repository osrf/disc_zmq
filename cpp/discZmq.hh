/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <arpa/inet.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dns_sd.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

#include "netUtils.hh"
#include "packet.hh"
#include "sockets/socket.hh"
#include "topicsInfo.hh"
#include "zmq/zmq.hpp"
#include "zmq/zmsg.hpp"

const int MaxRcvStr = 65536;  // Longest string to receive
const std::string InprocAddr = "inproc://local";

// #define LONG_TIME 100000000
 #define LONG_TIME 3

static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;

//  ---------------------------------------------------------------------
static void MyResolveCallBack(DNSServiceRef sdRef,
                              DNSServiceFlags flags,
                              uint32_t interfaceIndex,
                              DNSServiceErrorType errorCode,
                              const char *fullname,
                              const char *hosttarget,
                              uint16_t port,
                              uint16_t txtLen,
                              const unsigned char *txtRecord,
                              void *context);

//  ---------------------------------------------------------------------
void HandleEvents(DNSServiceRef serviceRef)
{
  int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
  int nfds = dns_sd_fd + 1;
  fd_set readfds;
  struct timeval tv;
  int result;

  while (!stopNow)
  {
    FD_ZERO(&readfds);
    FD_SET(dns_sd_fd, &readfds);
    tv.tv_sec = timeOut;
    tv.tv_usec = 0;
    result = select(nfds, &readfds, reinterpret_cast<fd_set*>(NULL),
      reinterpret_cast<fd_set*>(NULL), &tv);
    if (result > 0)
    {
      DNSServiceErrorType err = kDNSServiceErr_NoError;
      if (FD_ISSET(dns_sd_fd, &readfds))
        err = DNSServiceProcessResult(serviceRef);
      if (err) stopNow = 1;
    }
    else
    {
      std::cout << "Select() returned " << result << " errno " << errno <<
                   " " << strerror(errno) << std::endl;
      if (errno != EINTR) stopNow = 1;
    }
  }
}

//  ---------------------------------------------------------------------
static DNSServiceErrorType MyDNSServiceResolve(const std::string &_name,
                                               void *context)
{
  DNSServiceErrorType error;
  DNSServiceRef serviceRef;

  error = DNSServiceResolve(&serviceRef, 0, 0, _name.c_str(), "_discZmq._tcp",
    "", MyResolveCallBack, context);

  if (error == kDNSServiceErr_NoError)
  {
    HandleEvents(serviceRef);
    DNSServiceRefDeallocate(serviceRef);
  }
  else
    std::cerr << "MyDNSServiceResolve() returned " << error << std::endl;

  return error;
}

//  ---------------------------------------------------------------------
static void MyRegisterCallBack(DNSServiceRef service,
                               DNSServiceFlags flags,
                               DNSServiceErrorType errorCode,
                               const char *name,
                               const char *type,
                               const char *domain,
                               void *contest)
{
  #pragma unused(flags)
  #pragma unused(context)

  if (errorCode != kDNSServiceErr_NoError)
    std::cerr << "MyRegisterCallBack() returned " << errorCode << std::endl;
  else
    std::cout << "REGISTER " << name << " " << type << domain << "\n";
}

//  ---------------------------------------------------------------------
static void MyBrowseCallBack(DNSServiceRef service,
                             DNSServiceFlags flags,
                             uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode,
                             const char *name,
                             const char *type,
                             const char *domain,
                             void *context)
{
  // std::cout << "Callback()" << std::endl;

  if (errorCode != kDNSServiceErr_NoError)
    std::cerr << "MyBrowseCallBack returned " << errorCode << std::endl;
  else
  {
    const char *addStr = (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE";
    const char *moreStr = (flags & kDNSServiceFlagsMoreComing) ? "MORE" : "   ";
    // std::cout << addStr << " " << moreStr << " " << interfaceIndex
    //          << name << "." << type << domain << std::endl;
  }

  if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);

  if (flags & kDNSServiceFlagsAdd)
  {
    // ToDo: resolv this service and move the instructions below to the callback
    MyDNSServiceResolve(name, context);
  }
}

//  ---------------------------------------------------------------------
class Node
{
  public:
    //  ---------------------------------------------------------------------
    DNSServiceErrorType MyDNSServiceRegister(const std::string &_topic)
    {
      DNSServiceErrorType error;
      DNSServiceRef serviceRef;

      // Create the DNS TXT register data
      // char buffer[512];
      TXTRecordRef txtRecord;

      TXTRecordCreate(&txtRecord, 0, NULL);
      // Create a TXT record for the topic name
      //std::cout << "Topic: [" << _topic.data() << "]" << std::endl;
      TXTRecordSetValue(&txtRecord, "t", _topic.length(), _topic.data());
      // Create a TXT record for the GUID
      //std::cout << "GUID: [" << this->guidStr.data() << "]" << std::endl;
      TXTRecordSetValue(&txtRecord, "g", this->guidStr.size(),
        this->guidStr.data());
      // Create a TXT record for the current addresses
      std::string addresses = "";
      std::vector<std::string>::iterator it;
      for (it = this->myAddresses.begin(); it != this->myAddresses.end(); ++it)
        addresses += *it + " ";
      TXTRecordSetValue(&txtRecord, "a", addresses.size() - 1, addresses.data());
      //std::cout << "Addresses: [" << addresses << << "]" << std::endl;

      // Register the service
      error = DNSServiceRegister(&serviceRef,
                                 0,
                                 0,
                                 "caguero",
                                 "_discZmq._tcp",
                                 "",
                                 NULL,
                                 htons(9092),
                                 TXTRecordGetLength(&txtRecord),
                                 TXTRecordGetBytesPtr(&txtRecord),
                                 MyRegisterCallBack,
                                 NULL);

      TXTRecordDeallocate(&txtRecord);

      if (error == kDNSServiceErr_NoError)
      {
        HandleEvents(serviceRef);
        DNSServiceRefDeallocate(serviceRef);
      }

      return error;
    }

    //  ---------------------------------------------------------------------
    DNSServiceErrorType MyDNSServiceBrowse()
    {
      DNSServiceErrorType error;
      DNSServiceRef serviceRef;

      error = DNSServiceBrowse(&serviceRef, 0, 0, "_discZmq._tcp", "",
        MyBrowseCallBack, this);

      if (error == kDNSServiceErr_NoError)
      {
        // std::cout << "Browsing services" << std::endl;
        HandleEvents(serviceRef);
        //DNSServiceProcessResult(serviceRef);
        DNSServiceRefDeallocate(serviceRef);
      }
      else
        std::cerr << "MyDNSServiceBrowse() returned " << error << std::endl;

      return error;
    }

    //  ---------------------------------------------------------------------
    /// \brief Constructor.
    /// \param[in] _master End point with the master's endpoint.
    /// \param[in] _verbose true for enabling verbose mode.
    Node(std::string _master, bool _verbose)
    {
      char bindEndPoint[1024];

      // Remove avahi warnings
      setenv("AVAHI_COMPAT_NOWARN", "1", 1);

      // Initialize random seed
      srand(time(NULL));

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
        std::string anyTcpEP = "tcp://" + this->hostAddr + ":*";
        this->publisher->bind(anyTcpEP.c_str());
        size_t size = sizeof(bindEndPoint);
        this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
        this->publisher->bind(InprocAddr.c_str());
        this->tcpEndpoint = bindEndPoint;
        this->myAddresses.push_back(this->tcpEndpoint);
        this->subscriber->connect(InprocAddr.c_str());

        this->srvReplier->bind(anyTcpEP.c_str());
        this->srvReplier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
        this->srvReplierEP = bindEndPoint;
        this->mySrvAddresses.push_back(this->srvReplierEP);
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
        std::cout << "Bind at: [" << this->srvReplierEP << "] for reps\n";
        std::cout << "Bind at: [" << this->srvRequesterEP << "] for reqs\n";
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
    /// \brief Receive the next message.
    void SpinOnce()
    {
      this->SendPendingAsyncSrvCalls();

      //  Poll socket for a reply, with timeout
      zmq::pollitem_t items[] = {
        { *this->subscriber, 0, ZMQ_POLLIN, 0 },
        { *this->srvReplier, 0, ZMQ_POLLIN, 0 },
        { *this->srvRequester, 0, ZMQ_POLLIN, 0 }
        // { 0, this->bcastSock->sockDesc, ZMQ_POLLIN, 0 }
      };
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

      //  If we got a reply, process it
      if (items[0].revents & ZMQ_POLLIN)
        this->RecvTopicUpdates();
      else if (items[1].revents & ZMQ_POLLIN)
        this->RecvSrvRequest();
      else if (items[2].revents & ZMQ_POLLIN)
        this->RecvSrvReply();
      // else if (items[3].revents & ZMQ_POLLIN)
      //   this->RecvDiscoveryUpdates();
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
    /// \brief Advertise a new service.
    /// \param[in] _topic Topic to be advertised.
    /// \return 0 when success.
    int Advertise(const std::string &_topic)
    {
      assert(_topic != "");

      this->topics.SetAdvertisedByMe(_topic, true);

      /*std::vector<std::string>::iterator it;
      for (it = this->myAddresses.begin(); it != this->myAddresses.end(); ++it)
        this->SendAdvertiseMsg(ADV, _topic, *it);*/

      if (int error = this->MyDNSServiceRegister(_topic) != 0)
      {
        std::cout << "Error. ServiceRegister() returned " << error << std::endl;
        return error;
      }

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Unadvertise a new service.
    /// \param[in] _topic Topic to be unadvertised.
    /// \return 0 when success.
    int UnAdvertise(const std::string &_topic)
    {
      assert(_topic != "");

      this->topics.SetAdvertisedByMe(_topic, false);

      // ToDo: Unregister the topic in the dns_sd daemon

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Publish data.
    /// \param[in] _topic Topic to be published.
    /// \param[in] _data Data to publish.
    /// \return 0 when success.
    int Publish(const std::string &_topic, const std::string &_data)
    {
      assert(_topic != "");

      if (this->topics.AdvertisedByMe(_topic))
      {
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
      else
      {
        if (this->verbose)
          std::cerr << "\nNot published. (" << _topic << ") not advertised\n";
        return -1;
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a topic registering a callback.
    /// \param[in] _topic Topic to be subscribed.
    /// \param[in] _cb Pointer to the callback function.
    /// \return 0 when success.
    int Subscribe(const std::string &_topic,
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
      // return this->SendSubscribeMsg(SUB, _topic);
      this->MyDNSServiceBrowse();

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Subscribe to a topic registering a callback.
    /// \param[in] _topic Topic to be unsubscribed.
    /// \return 0 when success.
    int UnSubscribe(const std::string &_topic)
    {
      assert(_topic != "");
      if (this->verbose)
        std::cout << "\nUnubscribe (" << _topic << ")\n";

      this->topics.SetSubscribed(_topic, false);
      this->topics.SetCallback(_topic, NULL);

      // Remove the filter for this topic
      this->subscriber->setsockopt(ZMQ_UNSUBSCRIBE, _topic.data(),
                                  _topic.size());
      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Advertise a new service call registering a callback.
    /// \param[in] _topic Topic to be advertised.
    /// \param[in] _cb Pointer to the callback function.
    /// \return 0 when success.
    int SrvAdvertise(const std::string &_topic,
      int(*_cb)(const std::string &, const std::string &, std::string &))
    {
      assert(_topic != "");

      this->topicsSrvs.SetAdvertisedByMe(_topic, true);
      this->topicsSrvs.SetRepCallback(_topic, _cb);

      if (this->verbose)
        std::cout << "\nAdvertise srv call(" << _topic << ")\n";

      /*std::vector<std::string>::iterator it;
      for (it = this->mySrvAddresses.begin();
           it != this->mySrvAddresses.end(); ++it)
        this->SendAdvertiseMsg(ADV_SVC, _topic, *it);*/

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Unadvertise a service call registering a callback.
    /// \param[in] _topic Topic to be unadvertised.
    /// \return 0 when success.
    int SrvUnAdvertise(const std::string &_topic)
    {
      assert(_topic != "");

      this->topicsSrvs.SetAdvertisedByMe(_topic, false);
      this->topicsSrvs.SetRepCallback(_topic, NULL);

      if (this->verbose)
        std::cout << "\nUnadvertise srv call(" << _topic << ")\n";

      // ToDo: Notify the avahi daemon

      return 0;
    }

    //  ---------------------------------------------------------------------
    /// \brief Request a new service to another component using a blocking call.
    /// \param[in] _topic Topic requested.
    /// \param[in] _data Data of the request.
    /// \param[out] _response Response of the request.
    /// \return 0 when success.
    int SrvRequest(const std::string &_topic, const std::string &_data,
      std::string &_response)
    {
      /*assert(_topic != "");

      this->topicsSrvs.SetRequested(_topic, true);

      int retries = 25;
      while (!this->topicsSrvs.Connected(_topic) && retries > 0)
      {
        this->SendSubscribeMsg(SUB_SVC, _topic);
        this->SpinOnce();
        s_sleep(200);
        --retries;
      }

      if (!this->topicsSrvs.Connected(_topic))
        return -1;

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
      msg.send(*this->srvRequester);

      // Poll socket for a reply, with timeout
      zmq::pollitem_t items[] = { { *this->srvRequester, 0, ZMQ_POLLIN, 0 } };
      zmq::poll(items, 1, this->timeout);

      //  If we got a reply, process it
      if (items[0].revents & ZMQ_POLLIN)
      {
        zmsg *reply = new zmsg(*this->srvRequester);

        std::string((char*)reply->pop_front().c_str());
        std::string((char*)reply->pop_front().c_str());
        _response =
            std::string((char*)reply->pop_front().c_str());

        return 0;
      }*/

      return -1;
    }

    //  ---------------------------------------------------------------------
    /// \brief Request a new service call using a non-blocking call.
    /// \param[in] _topic Topic requested.
    /// \param[in] _data Data of the request.
    /// \param[in] _cb Pointer to the callback function.
    /// \return 0 when success.
    int SrvRequestAsync(const std::string &_topic, const std::string &_data,
      void(*_cb)(const std::string &_topic, int rc, const std::string &_rep))
    {
      /*assert(_topic != "");

      this->topicsSrvs.SetRequested(_topic, true);
      this->topicsSrvs.SetReqCallback(_topic, _cb);
      this->topicsSrvs.AddReq(_topic, _data);

      if (this->verbose)
        std::cout << "\nAsync request (" << _topic << ")" << std::endl;

      this->SendSubscribeMsg(SUB_SVC, _topic);*/

      return 0;
    }

  private:
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
    /// \brief Method in charge of receiving the topic updates.
    void RecvTopicUpdates()
    {
      zmsg *msg = new zmsg(*this->subscriber);
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
      std::string topic =
        std::string((char*)msg->pop_front().c_str());
      msg->pop_front();  // Sender
      std::string data =
        std::string((char*)msg->pop_front().c_str());

      if (this->topics.Subscribed(topic))
      {
        // Execute the callback registered
        TopicInfo::Callback cb;
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
      zmsg *msg = new zmsg(*this->srvReplier);
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
      std::string topic =
          std::string((char*)msg->pop_front().c_str());
      std::string((char*)msg->pop_front().c_str());
      std::string data =
          std::string((char*)msg->pop_front().c_str());

      if (this->topicsSrvs.AdvertisedByMe(topic))
      {
        // Execute the callback registered
        TopicInfo::RepCallback cb;
        std::string response;
        if (this->topicsSrvs.GetRepCallback(topic, cb))
          int rc = cb(topic, data, response);
        else
          std::cerr << "I don't have a REP cback for topic [" << topic << "]\n";

        // Send the service call response
        zmsg reply;
        reply.push_back((char*)topic.c_str());
        reply.push_back((char*)this->srvReplierEP.c_str());
        reply.push_back((char*)response.c_str());
        // Todo: include the return code

        if (this->verbose)
        {
          std::cout << "\nResponse (" << topic << ")" << std::endl;
          reply.dump();
        }
        reply.send(*this->srvReplier);
      }
      else
      {
        std::cerr << "Received a svc call not advertised (" << topic << ")\n";
      }
    }

    //  ---------------------------------------------------------------------
    /// \brief Method in charge of receiving the async service call replies.
    void RecvSrvReply()
    {
      zmsg *msg = new zmsg(*this->srvRequester);
      if (this->verbose)
      {
        std::cout << "\nReceived async service reply" << std::endl;
        msg->dump();
      }

      if (msg->parts() != 3)
      {
        std::cerr << "Unexpected service reply. Expected 3 message parts but "
                  << "received a message with " << msg->parts() << std::endl;
        return;
      }

      // Read the SRV_REP message
      std::string topic =
          std::string((char*)msg->pop_front().c_str());
      std::string((char*)msg->pop_front().c_str());
      std::string response =
          std::string((char*)msg->pop_front().c_str());

      // Execute the callback registered
      TopicInfo::ReqCallback cb;
      if (this->topicsSrvs.GetReqCallback(topic, cb))
        // ToDo: send the return code
        cb(topic, 0, response);
      else
        std::cerr << "REQ callback for topic [" << topic << "] not found\n";
    }

    //  ---------------------------------------------------------------------
    /// \brief Send all the pendings asynchronous service calls (if possible)
    void SendPendingAsyncSrvCalls()
    {
      // Check if there are any pending requests ready to send
      for (TopicInfo::Topics_M_it it = this->topicsSrvs.GetTopics().begin();
           it != this->topicsSrvs.GetTopics().end(); ++it)
      {
        std::string topic = it->first;

        if (!this->topicsSrvs.Connected(topic))
          continue;

        while (this->topicsSrvs.PendingReqs(topic))
        {
          std::string data;
          if (!this->topicsSrvs.DelReq(topic, data))
          {
            std::cerr << "Something went wrong removing a service request on "
                      << "topic (" << topic << ")\n";
            continue;
          }

          // Send the service call request
          zmsg msg;
          msg.push_back((char*)topic.c_str());
          msg.push_back((char*)this->srvRequesterEP.c_str());
          msg.push_back((char*)data.c_str());

          if (this->verbose)
          {
            std::cout << "\nAsync request [" << topic << "][" << data << "]\n";
            msg.dump();
          }
          msg.send(*this->srvRequester);
        }
      }
    }

    // Master address
    std::string master;

    // Print activity to stdout
    // int verbose;

    // Topic information
    // TopicsInfo topics;
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
    // zmq::socket_t *subscriber;    //  Socket to receive topic updates
    zmq::socket_t *srvRequester;  //  Socket to send service call requests
    zmq::socket_t *srvReplier;    //  Socket to receive service call requests
    std::string tcpEndpoint;
    std::string srvRequesterEP;
    std::string srvReplierEP;
    int timeout;                  //  Request timeout

    // GUID
    boost::uuids::uuid guid;
    // std::string guidStr;

  public:
    TopicsInfo topics;
    std::string guidStr;
    zmq::socket_t *subscriber;    //  Socket to receive topic updates
    int verbose;
};

//  ---------------------------------------------------------------------
static void MyResolveCallBack(DNSServiceRef sdRef,
                              DNSServiceFlags flags,
                              uint32_t interfaceIndex,
                              DNSServiceErrorType errorCode,
                              const char *fullname,
                              const char *hosttarget,
                              uint16_t port,
                              uint16_t txtLen,
                              const unsigned char *txtRecord,
                              void *context)
{
  uint8_t len;
  char *valuePtr;
  char *address, *topic, *rcvdGuid;
  // std::cout << "MyResolveCallBack()" << std::endl;

  Node *node = (Node*)context;

  if (TXTRecordContainsKey(txtLen, txtRecord, "a"))
  {
    // Read the address
    valuePtr = (char*)TXTRecordGetValuePtr(txtLen, txtRecord, "a", &len);
    address = new char[len + 1];
    bcopy(valuePtr, address, len);
    address[len] = '\0';
    // std::cout << "Address parsed: [" << address << "]" << std::endl;
  }

  if (TXTRecordContainsKey(txtLen, txtRecord, "t"))
  {
    // Read the topic
    valuePtr = (char*)TXTRecordGetValuePtr(txtLen, txtRecord, "t", &len);
    topic = new char[len + 1];
    bcopy(valuePtr, topic, len);
    topic[len] = '\0';
    // std::cout << "Topic parsed: [" << topic << "], len: "
    //          << static_cast<int>(len) << std::endl;
  }

  if (TXTRecordContainsKey(txtLen, txtRecord, "g"))
  {
    // Read the GUID
    valuePtr = (char*)TXTRecordGetValuePtr(txtLen, txtRecord, "g", &len);
    rcvdGuid = new char[len + 1];
    bcopy(valuePtr, rcvdGuid, len);
    rcvdGuid[len] = '\0';
    // std::cout << "GUIDparsed: [" << rcvdGuid << "]" << std::endl;
  }

  // Register the advertised address for the topic
  node->topics.AddAdvAddress(topic, address);

  // Check if we are interested in this topic
  if (node->topics.Subscribed(topic) &&
      !node->topics.Connected(topic) &&
      node->guidStr.compare(rcvdGuid) != 0)
  {
    try
    {
      node->subscriber->connect(address);
      node->topics.SetConnected(topic, true);
      if (node->verbose)
        std::cout << "\t* Connected to [" << address << "]\n";
    }
    catch(const zmq::error_t& ze)
    {
      std::cout << "Error connecting [" << ze.what() << "]\n";
    }
  }
}

#endif
