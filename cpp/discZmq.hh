#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <arpa/inet.h>
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

#include <avahi-compat-libdns_sd/dns_sd.h>

const int MaxRcvStr = 65536; // Longest string to receive
const std::string InprocAddr = "inproc://local";




/*#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>



static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static char *name = NULL;

static void create_services(AvahiClient *c);

void resolve_callback(AvahiServiceResolver *r,
                      AVAHI_GCC_UNUSED AvahiIfIndex interface,
                      AVAHI_GCC_UNUSED AvahiProtocol protocol,
                      AvahiResolverEvent event,
                      const char *name,
                      const char *type,
                      const char *domain,
                      const char *host_name,
                      const AvahiAddress *address,
                      uint16_t port,
                      AvahiStringList *txt,
                      AvahiLookupResultFlags flags,
                      AVAHI_GCC_UNUSED void* userdata)
{
  assert(r);

  // Called whenever a service has been resolved successfully or timed out
  switch (event) {
      case AVAHI_RESOLVER_FAILURE:
          fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
          break;

      case AVAHI_RESOLVER_FOUND: {
          char a[AVAHI_ADDRESS_STR_MAX], *t;

          fprintf(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);

          avahi_address_snprint(a, sizeof(a), address);
          t = avahi_string_list_to_string(txt);
          fprintf(stderr,
                  "\t%s:%u (%s)\n"
                  "\tTXT=%s\n"
                  "\tcookie is %u\n"
                  "\tis_local: %i\n"
                  "\tour_own: %i\n"
                  "\twide_area: %i\n"
                  "\tmulticast: %i\n"
                  "\tcached: %i\n",
                  host_name, port, a,
                  t,
                  avahi_string_list_get_service_cookie(txt),
                  !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
                  !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
                  !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
                  !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
                  !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

          avahi_free(t);
      }
  }

  avahi_service_resolver_free(r);
}

void browse_callback(AvahiServiceBrowser *b,
                     AvahiIfIndex interface,
                     AvahiProtocol protocol,
                     AvahiBrowserEvent event,
                     const char *type,
                     const char *domain,
                     AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                     void* userdata)
{

  AvahiClient *c = (AvahiClient *) userdata;
  assert(b);

  AvahiLookupFlags moreFlags;

  // Called whenever a new services becomes available on the LAN or is removed from the LAN

  switch (event) {
      case AVAHI_BROWSER_FAILURE:

          fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
          avahi_simple_poll_quit(simple_poll);
          return;

      case AVAHI_BROWSER_NEW:
          fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

          // We ignore the returned resolver object. In the callback
          //   function we free it. If the server is terminated before
          //   the callback function is called the server will free
          //   the resolver for us.

          if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, moreFlags, resolve_callback, c)))
              fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));

          break;

      case AVAHI_BROWSER_REMOVE:
          fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
          break;

      case AVAHI_BROWSER_ALL_FOR_NOW:
      case AVAHI_BROWSER_CACHE_EXHAUSTED:
          fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
          break;
  }
}

void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
{
  assert(g == group || group == NULL);
  group = g;

  // Called whenever the entry group state changes

  switch (state) {
      case AVAHI_ENTRY_GROUP_ESTABLISHED :
          // The entry group has been established successfully
          fprintf(stderr, "Service '%s' successfully established.\n", name);
          break;

      case AVAHI_ENTRY_GROUP_COLLISION : {
          char *n;

          // A service name collision with a remote service
          // happened. Let's pick a new name
          n = avahi_alternative_service_name(name);
          avahi_free(name);
          name = n;

          fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

          // And recreate the services
          create_services(avahi_entry_group_get_client(g));
          break;
      }

      case AVAHI_ENTRY_GROUP_FAILURE :

          fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

          // Some kind of failure happened while we were registering our services
          avahi_simple_poll_quit(simple_poll);
          break;

      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
          ;
  }
}

void create_services(AvahiClient *c)
{
  char *n, r[128];
  int ret;
  assert(c);
  AvahiPublishFlags flags = AVAHI_PUBLISH_USE_MULTICAST;

  // If this is the first time we're called, let's create a new
  // entry group if necessary

  if (!group)
  {
    std::cout << "Group is null\n";
    if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
        fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
        return;
    }
  }

  // If the group is empty (either because it was just created, or
  // because it was reset previously, add our entries.

  if (avahi_entry_group_is_empty(group)) {
      fprintf(stderr, "Adding service '%s'\n", name);

      // Create some random TXT data
      snprintf(r, sizeof(r), "random=%i", rand());

      // We will now add two services and one subtype to the entry
      // group. The two services have the same name, but differ in
      // the service type (IPP vs. BSD LPR). Only services with the
      // same name should be put in the same entry group.

      // Add the service for IPP
      if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, flags, name, "_ros._tcp", NULL, NULL, 2651, "test=blah", r, NULL)) < 0) {

          if (ret == AVAHI_ERR_COLLISION)
          {
            std::cerr << "Error 1, collision\n";
            return;
          }

          fprintf(stderr, "Failed to add _ros._tcp service: %s\n", avahi_strerror(ret));
          return;
      }

      // Add an additional (hypothetic) subtype
      // if ((ret = avahi_entry_group_add_service_subtype(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, flags, name, "_ros._tcp", NULL, "_magic._sub.ros._tcp") < 0)) {
      //    fprintf(stderr, "Failed to add subtype _magic._sub._printer._tcp: %s\n", avahi_strerror(ret));
      //   return;
      //}

      // Tell the server to register the service
      if ((ret = avahi_entry_group_commit(group)) < 0) {
          fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
          return;
      }
  }

  return;

//collision:

    // A service name collision with a local service happened. Let's
    // pick a new name
//    n = avahi_alternative_service_name(name);
  //  avahi_free(name);
   // name = n;

   // fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

  //  avahi_entry_group_reset(group);

  //  create_services(c);
   // return;

//fail:
 //   avahi_simple_poll_quit(simple_poll);
}*/

// DNS_SD
#define LONG_TIME 100000000

static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;

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
    result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
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
    std::cout << "REGISTER " << name << " " << type << "." << domain << "\n";
}

static DNSServiceErrorType MyDNSServiceRegister()
{
  DNSServiceErrorType error;
  DNSServiceRef serviceRef;

  error = DNSServiceRegister(&serviceRef,
                             0,
                             0,
                             "Not a real page",
                             "_http._tcp",
                             "",
                             NULL,
                             htons(9092),
                             0,
                             NULL,
                             MyRegisterCallBack,
                             NULL);

  if (error == kDNSServiceErr_NoError)
  {
    HandleEvents(serviceRef);
    DNSServiceRefDeallocate(serviceRef);
  }

  return error;
}


class Node
{
  public:
    /*void client_callback(AvahiClient *c, AvahiClientState state,
                         AVAHI_GCC_UNUSED void * userdata)
    {
      assert(c);

      // Called whenever the client or server state changes

      if (state == AVAHI_CLIENT_FAILURE) {
        fprintf(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
        avahi_simple_poll_quit(simple_poll);
      }
      else if (state = AVAHI_CLIENT_S_RUNNING)
      {
        std::cout << "Running\n";
        // The server has startup successfully and registered its host
        // name on the network, so it's time to create our services
        create_services(c);
      }
      else if (state = AVAHI_CLIENT_S_REGISTERING)
      {
        std::cout << "Registering\n";
        // The server records are now being established. This
        // might be caused by a host name change. We need to wait
        // for our own records to register until the host name is
        // properly esatblished.

        if (group)
            avahi_entry_group_reset(group);
      }
    }*/

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
      catch (const zmq::error_t& ze)
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
      zmq::pollitem_t items [] = {
        { *this->subscriber, 0, ZMQ_POLLIN, 0 },
        { *this->srvReplier, 0, ZMQ_POLLIN, 0 },
        { 0, this->bcastSock->sockDesc, ZMQ_POLLIN, 0 },
        { *this->srvRequester, 0, ZMQ_POLLIN, 0 }
      };
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

      //  If we got a reply, process it
      if (items [0].revents & ZMQ_POLLIN)
        this->RecvTopicUpdates();
      else if (items [1].revents & ZMQ_POLLIN)
        this->RecvSrvRequest();
      else if (items [2].revents & ZMQ_POLLIN)
        this->RecvDiscoveryUpdates();
      else if (items [3].revents & ZMQ_POLLIN)
        this->RecvSrvReply();
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

      /*AvahiClient *client = NULL;
      int error;
      struct timeval tv;
      AvahiClientFlags flags;

      // Allocate main loop object
      if (!(simple_poll = avahi_simple_poll_new())) {
          fprintf(stderr, "Failed to create simple poll object.\n");
          return 1;
      }

      name = avahi_strdup("MegaPrinter");

      // Allocate a new client
      client = avahi_client_new(avahi_simple_poll_get(simple_poll), flags, this->client_callback, NULL, &error);

      // Check wether creating the client object succeeded
      if (!client) {
          fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
          return 1;
      }*/

      DNSServiceErrorType error = MyDNSServiceRegister();
      std::cout << "ServiceRegister() returned " << error << std::endl;

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
      return this->SendSubscribeMsg(SUB, _topic);
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

      std::vector<std::string>::iterator it;
      for (it = this->mySrvAddresses.begin();
           it != this->mySrvAddresses.end(); ++it)
        this->SendAdvertiseMsg(ADV_SVC, _topic, *it);

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
      assert(_topic != "");

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
    /// \return 0 when success.
    int SrvRequestAsync(const std::string &_topic, const std::string &_data,
      void(*_cb)(const std::string &_topic, int rc, const std::string &_rep))
    {
      assert(_topic != "");

      this->topicsSrvs.SetRequested(_topic, true);
      this->topicsSrvs.SetReqCallback(_topic, _cb);
      this->topicsSrvs.AddReq(_topic, _data);

      if (this->verbose)
        std::cout << "\nAsync request (" << _topic << ")" << std::endl;

      this->SendSubscribeMsg(SUB_SVC, _topic);

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
    /// \brief Method in charge of receiving the discovery updates.
    void RecvDiscoveryUpdates()
    {
      char rcvStr[MaxRcvStr];     // Buffer for data
      std::string srcAddr;           // Address of datagram source
      unsigned short srcPort;        // Port of datagram source
      int bytes;                 // Rcvd from the UDP broadcast socket

      try
      {
        bytes = this->bcastSock->recvFrom(rcvStr, MaxRcvStr, srcAddr, srcPort);
      } catch (SocketException &e)
      {
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
      std::string sender = std::string((char*)msg->pop_front().c_str());
      std::string data = std::string((char*)msg->pop_front().c_str());

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
        reply.send (*this->srvReplier);
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
      zmsg *msg = new zmsg (*this->srvRequester);
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
      std::string topic = std::string((char*)msg->pop_front().c_str());
      std::string((char*)msg->pop_front().c_str());
      std::string response = std::string((char*)msg->pop_front().c_str());

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
           it != this->topicsSrvs.GetTopics().end(); it++)
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
          msg.send (*this->srvRequester);
        }
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

      Header header(TRNSP_VERSION, this->guid, _topic, _type, 0);
      AdvMsg advMsg(header, _address);

      char *buffer = new char[advMsg.GetMsgLength()];
      advMsg.Pack(buffer);

      // Send the data through the UDP broadcast socket
      try
      {
        this->bcastSock->sendTo(buffer, advMsg.GetMsgLength(),
          this->bcastAddr, this->bcastPort);
      } catch (SocketException &e) {
        cerr << "Exception sending an ADV msg: " << e.what() << endl;
        delete[] buffer;
        return -1;
      }

      delete[] buffer;
      return 0;
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

      Header header(TRNSP_VERSION, this->guid, _topic, _type, 0);

      char *buffer = new char[header.GetHeaderLength()];
      header.Pack(buffer);

      // Send the data through the UDP broadcast socket
      try
      {
        this->bcastSock->sendTo(buffer, header.GetHeaderLength(),
          this->bcastAddr, this->bcastPort);
      } catch (SocketException &e) {
        cerr << "Exception sending a SUB msg: " << e.what() << endl;
        delete[] buffer;
        return -1;
      }

      delete[] buffer;
      return 0;
    }

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
    std::string srvRequesterEP;
    std::string srvReplierEP;
    int timeout;                  //  Request timeout

    // GUID
    boost::uuids::uuid guid;
    std::string guidStr;
};

#endif
