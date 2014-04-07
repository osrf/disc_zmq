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

#include <algorithm>
#include <string>
#include "topicsInfo.hh"

//////////////////////////////////////////////////
transport::TopicInfo::TopicInfo()
{
  this->connected      = false;
  this->subscribed     = false;
  this->advertisedByMe = false;
  this->requested      = false;
  this->cb             = nullptr;
  this->reqCb          = nullptr;
  this->repCb          = nullptr;
}

//////////////////////////////////////////////////
transport::TopicInfo::~TopicInfo()
{
  this->addresses.clear();
  this->pendingReqs.clear();
}

//////////////////////////////////////////////////
transport::TopicsInfo::TopicsInfo()
{
}

//////////////////////////////////////////////////
transport::TopicsInfo::~TopicsInfo()
{
  for (auto topicInfo : this->GetTopicsInfo())
    delete topicInfo.second;

  this->topicsInfo.clear();
};

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasTopic(const std::string &_topic)
{
  return this->topicsInfo.find(_topic) != this->topicsInfo.end();
}


//////////////////////////////////////////////////
bool transport::TopicsInfo::GetAdvAddresses(const std::string &_topic,
                                            TopicInfo::Topics_L &_addresses)
{
  if (!this->HasTopic(_topic))
    return false;

  _addresses = topicsInfo[_topic]->addresses;
  return true;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasAdvAddress(const std::string &_topic,
                                          const std::string &_address)
{
  if (!this->HasTopic(_topic))
    return false;

  return std::find(this->topicsInfo[_topic]->addresses.begin(),
            this->topicsInfo[_topic]->addresses.end(), _address)
              != this->topicsInfo[_topic]->addresses.end();
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::Connected(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->connected;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::Subscribed(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscribed;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::AdvertisedByMe(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->advertisedByMe;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::Requested(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->requested;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetCallback(const std::string &_topic,
                                        TopicInfo::Callback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->cb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetReqCallback(const std::string &_topic,
                                           TopicInfo::ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->reqCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetRepCallback(const std::string &_topic,
                                           TopicInfo::RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->repCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::PendingReqs(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->pendingReqs.empty();
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddAdvAddress(const std::string &_topic,
                                          const std::string &_address)
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

//////////////////////////////////////////////////
void transport::TopicsInfo::RemoveAdvAddress(const std::string &_topic,
                                             const std::string &_address)
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

//////////////////////////////////////////////////
void transport::TopicsInfo::SetConnected(const std::string &_topic,
                                         const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->connected = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetSubscribed(const std::string &_topic,
                                          const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->subscribed = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetRequested(const std::string &_topic,
                                         const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->requested = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetAdvertisedByMe(const std::string &_topic,
                                              const bool _value)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->advertisedByMe = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetCallback(const std::string &_topic,
                                        const TopicInfo::Callback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->cb = _cb;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetReqCallback(const std::string &_topic,
                                           const TopicInfo::ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->reqCb = _cb;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetRepCallback(const std::string &_topic,
                                           const TopicInfo::RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->repCb = _cb;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddReq(const std::string &_topic,
                                   const std::string &_data)
{
  if (!this->HasTopic(_topic))
  {
    TopicInfo *topicInfo = new TopicInfo();
    this->topicsInfo.insert(make_pair(_topic, topicInfo));
  }

  this->topicsInfo[_topic]->pendingReqs.push_back(_data);
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::DelReq(const std::string &_topic,
                                   std::string &_data)
{
  if (!this->HasTopic(_topic))
    return false;

  if (this->topicsInfo[_topic]->pendingReqs.empty())
    return false;

  _data = this->topicsInfo[_topic]->pendingReqs.front();
  this->topicsInfo[_topic]->pendingReqs.pop_front();
  return true;
}

//////////////////////////////////////////////////
transport::TopicInfo::Topics_M& transport::TopicsInfo::GetTopicsInfo()
{
  return this->topicsInfo;
}
