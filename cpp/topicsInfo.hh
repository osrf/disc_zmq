/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef __TOPICS_INFO_HH_INCLUDED__
#define __TOPICS_INFO_HH_INCLUDED__

#include <boost/function.hpp>
#include <list>
#include <map>
#include <string>
#include <vector>

// Info about a topic for pub/sub
class TopicInfo
{
  public:
    // Topic list
    typedef std::vector<std::string> Topics_L;

    typedef boost::function<void (const std::string &,
                                  const std::string &)> Callback;
    typedef boost::function<void (const std::string &, int,
                                  const std::string &)> ReqCallback;
    typedef boost::function<int (const std::string &, const std::string &,
                                 std::string &)> RepCallback;

    typedef std::map<std::string, TopicInfo*> Topics_M;

    typedef Topics_M::iterator Topics_M_it;

    TopicInfo();

    virtual ~TopicInfo();

  public:
    Topics_L addresses;
    bool connected;
    bool advertisedByMe;

    // Used by pub/sub
    bool subscribed;
    Callback cb;

    // Used by service calls
    bool requested;
    ReqCallback reqCb;
    RepCallback repCb;
    std::list<std::string> pendingReqs;
};

class TopicsInfo
{
  public:
  /// \brief Constructor.
  TopicsInfo();

  /// \brief Destructor.
  virtual ~TopicsInfo();

  /// \brief Return if there is some information about a topic stored.
  /// \param[in] _topic Topic name.
  /// \return true If there is information about the topic.
  bool HasTopic(const std::string &_topic);

  /// \brief Get the known list of addresses associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[out] _addresses List of addresses
  /// \return true when we have info about this topic.
  bool GetAdvAddresses(const std::string &_topic,
                       std::vector<std::string> &_addresses);

  /// \brief Return if an address is registered associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _address Address to check.
  /// \return true if the address is registered with the topic.
  bool HasAdvAddress(const std::string &_topic, const std::string &_address);

  /// \brief Return true if we are connected to a node advertising the topic.
  /// \param[in] _topic Topic name.
  /// \return true if we are connected.
  bool Connected(const std::string &_topic);

  /// \brief Return true if we are subscribed to a node advertising the topic.
  /// \param[in] _topic Topic name.
  /// \return true if we are subscribed.
  bool Subscribed(const std::string &_topic);

  /// \brief Return true if I am advertising the topic.
  /// \param[in] _topic Topic name.
  /// \return true if the topic is advertised by me.
  bool AdvertisedByMe(const std::string &_topic);

  /// \brief Return true if I am requesting the service call associated to
  /// the topic.
  /// \param[in] _topic Topic name.
  /// \return true if the service call associated to the topic is requested.
  bool Requested(const std::string &_topic);

  /// \brief Get the callback associated to a topic subscription.
  /// \param[in] _topic Topic name.
  /// \param[out] A pointer to the function registered for a topic.
  /// \return true if there is a callback registered for the topic.
  bool GetCallback(const std::string &_topic, TopicInfo::Callback &_cb);

  /// \brief Get the REQ callback associated to a topic subscription.
  /// \param[in] _topic Topic name.
  /// \param[out] A pointer to the REQ function registered for a topic.
  /// \return true if there is a REQ callback registered for the topic.
  bool GetReqCallback(const std::string &_topic, TopicInfo::ReqCallback &_cb);

  /// \brief Get the REP callback associated to a topic subscription.
  /// \param[in] _topic Topic name.
  /// \param[out] A pointer to the REP function registered for a topic.
  /// \return true if there is a REP callback registered for the topic.
  bool GetRepCallback(const std::string &_topic, TopicInfo::RepCallback &_cb);

  /// \brief Returns if there are any pending requests in the queue.
  /// \param[in] _topic Topic name.
  /// \return true if there is any pending request in the queue.
  bool PendingReqs(const std::string &_topic);

  /// \brief Add a new address associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _address New address
  void AddAdvAddress(const std::string &_topic, const std::string &_address);

  /// \brief Remove an address associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[int] _address Address to remove.
  void RemoveAdvAddress(const std::string &_topic, const std::string &_address);

  /// \brief Set a new connected value to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _value New value to be assigned to the connected member.
  void SetConnected(const std::string &_topic, const bool _value);

  /// \brief Set a new subscribed value to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _value New value to be assigned to the subscribed member.
  void SetSubscribed(const std::string &_topic, const bool _value);

  /// \brief Set a new service call request to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _value New value to be assigned to the requested member.
  void SetRequested(const std::string &_topic, const bool _value);

  /// \brief Set a new advertised value to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _value New value to be assigned in advertisedByMe member.
  void SetAdvertisedByMe(const std::string &_topic, const bool _value);

  /// \brief Set a new callback associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _cb New callback.
  void SetCallback(const std::string &_topic, const TopicInfo::Callback &_cb);

  /// \brief Set a new REQ callback associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _cb New callback.
  void SetReqCallback(const std::string &_topic,
                      const TopicInfo::ReqCallback &_cb);

  /// \brief Set a new REP callback associated to a given topic.
  /// \param[in] _topic Topic name.
  /// \param[in] _cb New callback.
  void SetRepCallback(const std::string &_topic,
                      const TopicInfo::RepCallback &_cb);

  /// \brief Add a new service call request to the queue.
  /// \param[in] _topic Topic name.
  /// \param[in] _data Parameters of the request.
  void AddReq(const std::string &_topic, const std::string &_data);

  /// \brief Add a new service call request to the queue.
  /// \param[in] _topic Topic name.
  /// \param[out] _data Parameters of the request.
  /// \return true if a request was removed.
  bool DelReq(const std::string &_topic, std::string &_data);

  /// \brief Get a reference to the topics map.
  /// \return Reference to the topic map.
  TopicInfo::Topics_M& GetTopics();

  private:
    // Hash with the topic/topicInfo information for pub/sub
    TopicInfo::Topics_M topicsInfo;
};

#endif
