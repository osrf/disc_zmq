#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <map>
#include <string>

#include "transport_defs.hh"
#include "zmq/zmq.hpp"
#include "zmq/zmsg.hpp"

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

    //  ---------------------------------------------------------------------
    /// \brief Thread in charge of receiving the discovery updates.
    void RecvDiscoveryUpdates()
    {
      try
      {
        while (true)
        {
          s_sleep(1000);
        }
      }
      catch (boost::thread_interrupted& interruption)
      {
        // thread was interrupted, this is expected.
        std::cout << "Thread interrupted" << std::endl;
      }
      catch (std::exception& e)
      {
        // an unhandled exception reached this point, this constitutes an error

      }

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
};

#endif
