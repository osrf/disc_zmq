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

#include <limits.h>
#include <string>
#include <vector>
#include "../topicsInfo.hh"
#include "gtest/gtest.h"

//////////////////////////////////////////////////
void myCb(const std::string &p1, const std::string &p2)
{
}

//////////////////////////////////////////////////
void myReqCb(const std::string &p1, int p2, const std::string &p3)
{
}

//////////////////////////////////////////////////
int myRepCb(const std::string &p1, const std::string &p2, std::string &p3)
{
}

//////////////////////////////////////////////////
TEST(PacketTest, BasicTopicsInfoAPI)
{
  transport::TopicsInfo topics;
  std::string topic = "test_topic";
  std::string address = "tcp://10.0.0.1:6000";
  std::vector<std::string> v;
  transport::TopicInfo::Callback cb;
  transport::TopicInfo::ReqCallback reqCb;
  transport::TopicInfo::RepCallback repCb;

  // Check getters with an empty TopicsInfo object
  EXPECT_FALSE(topics.HasTopic(topic));
  EXPECT_FALSE(topics.GetAdvAddresses(topic, v));
  EXPECT_FALSE(topics.HasAdvAddress(topic, address));
  EXPECT_FALSE(topics.Connected(topic));
  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetCallback(topic, cb));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Check getters after inserting a topic in a TopicsInfo object
  topics.AddAdvAddress(topic, address);
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.HasAdvAddress(topic, address));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, v));
  EXPECT_EQ(v.at(0), address);
  EXPECT_FALSE(topics.Connected(topic));
  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetCallback(topic, cb));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Check that there's only one copy stored of the same address
  topics.AddAdvAddress(topic, address);
  EXPECT_TRUE(topics.GetAdvAddresses(topic, v));
  EXPECT_EQ(v.size(), 1);

  // Check SetConnected
  topics.SetConnected(topic, true);
  EXPECT_TRUE(topics.Connected(topic));

  // Check SetSubscribed
  topics.SetSubscribed(topic, true);
  EXPECT_TRUE(topics.Subscribed(topic));

  // Check SetRequested
  topics.SetRequested(topic, true);
  EXPECT_TRUE(topics.Requested(topic));

  // Check SetAdvertisedByMe
  topics.SetAdvertisedByMe(topic, true);
  EXPECT_TRUE(topics.AdvertisedByMe(topic));

  // Check SetCallback
  topics.SetCallback(topic, myCb);
  EXPECT_TRUE(topics.GetCallback(topic, cb));
  EXPECT_EQ(&myCb, cb);

  // Check SetReqCallback
  topics.SetReqCallback(topic, myReqCb);
  EXPECT_TRUE(topics.GetReqCallback(topic, reqCb));
  EXPECT_EQ(&myReqCb, reqCb);

  // Check SetRepCallback
  topics.SetRepCallback(topic, myRepCb);
  EXPECT_TRUE(topics.GetRepCallback(topic, repCb));
  EXPECT_EQ(&myRepCb, repCb);

  // Check the address removal
  topics.RemoveAdvAddress(topic, address);
  EXPECT_FALSE(topics.HasAdvAddress(topic, address));
  EXPECT_FALSE(topics.GetAdvAddresses(topic, v));

  // Check the addition of asynchronous service call requests
  std::string param1 = "param1";
  std::string param2 = "param2";
  EXPECT_FALSE(topics.DelReq(topic, param1));
  for (transport::TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
    EXPECT_FALSE(topics.PendingReqs(it->first));
  topics.AddReq(topic, param1);
  for (transport::TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
    EXPECT_TRUE(topics.PendingReqs(it->first));
  topics.AddReq(topic, param2);
  for (transport::TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
    EXPECT_TRUE(topics.PendingReqs(it->first));

  EXPECT_TRUE(topics.DelReq(topic, param2));
  EXPECT_TRUE(topics.DelReq(topic, param1));
  EXPECT_FALSE(topics.DelReq(topic, param1));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
