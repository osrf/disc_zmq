#include <limits.h>
#include "../topicsInfo.hh"
#include "gtest/gtest.h"

void myCb(const std::string &p1, const std::string &p2)
{
}

void myReqCb(const std::string &p1, int p2, const std::string &p3)
{
}

int myRepCb(const std::string &p1, const std::string &p2, std::string &p3)
{
}

TEST(PacketTest, BasicTopicsInfoAPI)
{
  TopicsInfo topics;
  std::string topic = "test_topic";
  std::string address = "tcp://10.0.0.1:6000";
  std::vector<std::string> v;
  TopicInfo::Callback cb;
  TopicInfo::ReqCallback reqCb;
  TopicInfo::RepCallback repCb;

  // Check getters with an empty TopicsInfo object
  EXPECT_EQ(topics.HasTopic(topic), false);
  EXPECT_EQ(topics.GetAdvAddresses(topic, v), false);
  EXPECT_EQ(topics.HasAdvAddress(topic, address), false);
  EXPECT_EQ(topics.Connected(topic), false);
  EXPECT_EQ(topics.Subscribed(topic), false);
  EXPECT_EQ(topics.AdvertisedByMe(topic), false);
  EXPECT_EQ(topics.Requested(topic), false);
  EXPECT_EQ(topics.GetCallback(topic, cb), false);
  EXPECT_EQ(topics.GetReqCallback(topic, reqCb), false);
  EXPECT_EQ(topics.GetRepCallback(topic, repCb), false);
  EXPECT_EQ(topics.PendingReqs(topic), false);

  // Check getters after inserting a topic in a TopicsInfo object
  topics.AddAdvAddress(topic, address);
  EXPECT_EQ(topics.HasTopic(topic), true);
  EXPECT_EQ(topics.HasAdvAddress(topic, address), true);
  EXPECT_EQ(topics.GetAdvAddresses(topic, v), true);
  EXPECT_EQ(v.at(0), address);
  EXPECT_EQ(topics.Connected(topic), false);
  EXPECT_EQ(topics.Subscribed(topic), false);
  EXPECT_EQ(topics.AdvertisedByMe(topic), false);
  EXPECT_EQ(topics.Requested(topic), false);
  EXPECT_EQ(topics.GetCallback(topic, cb), false);
  EXPECT_EQ(topics.GetReqCallback(topic, reqCb), false);
  EXPECT_EQ(topics.GetRepCallback(topic, repCb), false);
  EXPECT_EQ(topics.PendingReqs(topic), false);

  // Check that there's only one copy stored of the same address
  topics.AddAdvAddress(topic, address);
  EXPECT_EQ(topics.GetAdvAddresses(topic, v), true);
  EXPECT_EQ(v.size(), 1);

  // Check SetConnected
  topics.SetConnected(topic, true);
  EXPECT_EQ(topics.Connected(topic), true);

  // Check SetSubscribed
  topics.SetSubscribed(topic, true);
  EXPECT_EQ(topics.Subscribed(topic), true);

  // Check SetRequested
  topics.SetRequested(topic, true);
  EXPECT_EQ(topics.Requested(topic), true);

  // Check SetAdvertisedByMe
  topics.SetAdvertisedByMe(topic, true);
  EXPECT_EQ(topics.AdvertisedByMe(topic), true);

  // Check SetCallback
  topics.SetCallback(topic, myCb);
  EXPECT_EQ(topics.GetCallback(topic, cb), true);
  EXPECT_EQ(&myCb, cb);

	// Check SetReqCallback
  topics.SetReqCallback(topic, myReqCb);
  EXPECT_EQ(topics.GetReqCallback(topic, reqCb), true);
  EXPECT_EQ(&myReqCb, reqCb);

	// Check SetRepCallback
  topics.SetRepCallback(topic, myRepCb);
  EXPECT_EQ(topics.GetRepCallback(topic, repCb), true);
  EXPECT_EQ(&myRepCb, repCb);

  // Check the address removal
  topics.RemoveAdvAddress(topic, address);
  EXPECT_EQ(topics.HasAdvAddress(topic, address), false);
  EXPECT_EQ(topics.GetAdvAddresses(topic, v), false);

  // Check the addition of asynchronous service call requests
  std::string param1 = "param1";
  std::string param2 = "param2";
  EXPECT_EQ(topics.DelReq(topic, param1), false);
  for (TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
  	EXPECT_EQ(topics.PendingReqs(it->first), false);
  topics.AddReq(topic, param1);
  for (TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
  	EXPECT_EQ(topics.PendingReqs(it->first), true);
  topics.AddReq(topic, param2);
  for (TopicInfo::Topics_M_it it = topics.GetTopics().begin();
       it != topics.GetTopics().end(); ++it)
  	EXPECT_EQ(topics.PendingReqs(it->first), true);

  EXPECT_EQ(topics.DelReq(topic, param2), true);
  EXPECT_EQ(topics.DelReq(topic, param1), true);
  EXPECT_EQ(topics.DelReq(topic, param1), false);
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
