#include <boost/algorithm/string.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "topicsInfo.hh"

//  ---------------------------------------------------------------------
TopicInfo::TopicInfo()
{
  this->connected = false;
  this->subscribed = false;
  this->advertisedByMe = false;
  this->requested = false;
  this->cb = NULL;
  this->reqCb = NULL;
  this->repCb = NULL;
}

//  ---------------------------------------------------------------------
TopicInfo::~TopicInfo()
{
  this->addresses.clear();
  this->pendingReqs.clear();
}

//  ---------------------------------------------------------------------
TopicsInfo::TopicsInfo(){};

//  ---------------------------------------------------------------------
TopicsInfo::~TopicsInfo()
{
  for (TopicInfo::Topics_M_it it = this->GetTopics().begin();
    it != this->GetTopics().end(); ++it)
    delete it->second;

  this->topicsInfo.clear();
};

//  ---------------------------------------------------------------------
bool TopicsInfo::HasTopic(const std::string &_topic)
{
  return this->topicsInfo.find(_topic) != this->topicsInfo.end();
}


//  ---------------------------------------------------------------------
bool TopicsInfo::GetAdvAddresses(const std::string &_topic,
                     std::vector<std::string> &_addresses)
{
  if (!this->HasTopic(_topic))
    return false;

  _addresses = topicsInfo[_topic]->addresses;
  return true;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::HasAdvAddress(const std::string &_topic, const std::string &_address)
{
  if (!this->HasTopic(_topic))
    return false;

  return std::find(this->topicsInfo[_topic]->addresses.begin(),
            this->topicsInfo[_topic]->addresses.end(), _address)
              != this->topicsInfo[_topic]->addresses.end();
}

//  ---------------------------------------------------------------------
bool TopicsInfo::Connected(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->connected;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::Subscribed(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscribed;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::AdvertisedByMe(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->advertisedByMe;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::Requested(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->requested;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::GetCallback(const std::string &_topic, TopicInfo::Callback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->cb;
  return _cb != NULL;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::GetReqCallback(const std::string &_topic, TopicInfo::ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->reqCb;
  return _cb != NULL;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::GetRepCallback(const std::string &_topic, TopicInfo::RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->repCb;
  return _cb != NULL;
}

//  ---------------------------------------------------------------------
bool TopicsInfo::PendingReqs(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->pendingReqs.empty();
}

//  ---------------------------------------------------------------------
void TopicsInfo::AddAdvAddress(const std::string &_topic, const std::string &_address)
{
  // If we don't have the topic registered, add a new TopicInfo
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  // If we had the topic but not the address, add the new address
  if (!this->HasAdvAddress(_topic, _address))
    this->topicsInfo[_topic]->addresses.push_back(_address);
}

//  ---------------------------------------------------------------------
void TopicsInfo::RemoveAdvAddress(const std::string &_topic, const std::string &_address)
{
  // Remove the address if we have the topic
  if (this->HasTopic(_topic))
  {
    this->topicsInfo[_topic]->addresses.resize(
      std::remove(this->topicsInfo[_topic]->addresses.begin(),
        this->topicsInfo[_topic]->addresses.end(), _address) -
          this->topicsInfo[_topic]->addresses.begin());

    // If the addresses list is empty, just remove the topic info
    if (this->topicsInfo[_topic]->addresses.empty())
      this->topicsInfo.erase(_topic);
  }
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetConnected(const std::string &_topic, const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->connected = _value;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetSubscribed(const std::string &_topic, const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->subscribed = _value;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetRequested(const std::string &_topic, const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->requested = _value;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetAdvertisedByMe(const std::string &_topic, const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->advertisedByMe = _value;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetCallback(const std::string &_topic, const TopicInfo::Callback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->cb = _cb;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetReqCallback(const std::string &_topic,
                    const TopicInfo::ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->reqCb = _cb;
}

//  ---------------------------------------------------------------------
void TopicsInfo::SetRepCallback(const std::string &_topic,
                    const TopicInfo::RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->repCb = _cb;
}

//  ---------------------------------------------------------------------
void TopicsInfo::AddReq(const std::string &_topic, const std::string &_data)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->pendingReqs.push_back(_data);
}

//  ---------------------------------------------------------------------
bool TopicsInfo::DelReq(const std::string &_topic, std::string &_data)
{
  if (!this->HasTopic(_topic))
    return false;

  if (this->topicsInfo[_topic]->pendingReqs.empty())
    return false;

  _data = this->topicsInfo[_topic]->pendingReqs.front();
  this->topicsInfo[_topic]->pendingReqs.pop_front();
  return true;
}

//  ---------------------------------------------------------------------
TopicInfo::Topics_M& TopicsInfo::GetTopics()
{
  return this->topicsInfo;
}
