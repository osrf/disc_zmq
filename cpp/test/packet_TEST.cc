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
#include <boost/lexical_cast.hpp>

#include "../packet.hh"
#include "gtest/gtest.h"

//////////////////////////////////////////////////
TEST(PacketTest, BasicHeaderAPI)
{
  boost::uuids::uuid guid = boost::uuids::random_generator()();
  std::string guidStr = boost::lexical_cast<std::string>(guid);
  std::string topic = "topic_test";

  transport::Header header(TRNSP_VERSION, guid, topic, ADV, 0);

  // Check Header getters
  EXPECT_EQ(header.GetVersion(), TRNSP_VERSION);
  std::string otherGuidStr = boost::lexical_cast<std::string>(header.GetGuid());
  EXPECT_EQ(guidStr, otherGuidStr);
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetType(), ADV);
  EXPECT_EQ(header.GetFlags(), 0);
  int headerLength = sizeof(header.GetVersion()) + sizeof(header.GetGuid()) +
    sizeof(header.GetTopicLength()) + topic.size() + sizeof(header.GetType()) +
    sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  // Check Header setters
  header.SetVersion(TRNSP_VERSION + 1);
  EXPECT_EQ(header.GetVersion(), TRNSP_VERSION + 1);
  guid = boost::uuids::random_generator()();
  guidStr = boost::lexical_cast<std::string>(guid);
  header.SetGuid(guid);
  otherGuidStr = boost::lexical_cast<std::string>(header.GetGuid());
  EXPECT_EQ(guidStr, otherGuidStr);
  topic = "a_new_topic_test";
  header.SetTopic(topic);
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  header.SetType(SUB);
  EXPECT_EQ(header.GetType(), SUB);
  header.SetFlags(1);
  EXPECT_EQ(header.GetFlags(), 1);
  headerLength = sizeof(header.GetVersion()) + sizeof(header.GetGuid()) +
    sizeof(header.GetTopicLength()) + topic.size() + sizeof(header.GetType()) +
    sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);
}

//////////////////////////////////////////////////
TEST(PacketTest, HeaderIO)
{
  std::string guidStr;
  std::string otherGuidStr;

  boost::uuids::uuid guid = boost::uuids::random_generator()();
  std::string topic = "topic_test";

  // Pack a Header
  transport::Header header(TRNSP_VERSION, guid, topic, ADV_SVC, 2);
  char *buffer = new char[header.GetHeaderLength()];
  size_t bytes = header.Pack(buffer);
  EXPECT_EQ(bytes, header.GetHeaderLength());

  // Unpack the Header
  transport::Header otherHeader;
  otherHeader.Unpack(buffer);
  delete[] buffer;

  // Check that after Pack() and Unpack() the Header remains the same
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  guidStr = boost::lexical_cast<std::string>(header.GetGuid());
  otherGuidStr = boost::lexical_cast<std::string>(otherHeader.GetGuid());
  EXPECT_EQ(guidStr, otherGuidStr);
  EXPECT_EQ(header.GetTopicLength(), otherHeader.GetTopicLength());
  EXPECT_EQ(header.GetTopic(), otherHeader.GetTopic());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());
}

//////////////////////////////////////////////////
TEST(PacketTest, BasicAdvMsgAPI)
{
  boost::uuids::uuid guid = boost::uuids::random_generator()();
  std::string otherGuidStr = boost::lexical_cast<std::string>(guid);
  std::string topic = "topic_test";

  transport::Header otherHeader(TRNSP_VERSION, guid, topic, ADV, 3);
  std::string address = "tcp://10.0.0.1:6000";
  transport::AdvMsg advMsg(otherHeader, address);

  // Check AdvMsg getters
  transport::Header header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  std::string guidStr = boost::lexical_cast<std::string>(header.GetGuid());
  EXPECT_EQ(guidStr, otherGuidStr);
  EXPECT_EQ(header.GetTopicLength(), otherHeader.GetTopicLength());
  EXPECT_EQ(header.GetTopic(), otherHeader.GetTopic());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advMsg.GetAddressLength(), address.size());
  EXPECT_EQ(advMsg.GetAddress(), address);
  size_t msgLength = advMsg.GetHeader().GetHeaderLength() +
    sizeof(advMsg.GetAddressLength()) + advMsg.GetAddress().size();
  EXPECT_EQ(advMsg.GetMsgLength(), msgLength);

  guid = boost::uuids::random_generator()();
  guidStr = boost::lexical_cast<std::string>(guid);
  topic = "a_new_topic_test";

  // Check AdvMsg setters
  transport::Header anotherHeader(TRNSP_VERSION + 1, guid, topic, ADV_SVC, 3);
  advMsg.SetHeader(anotherHeader);
  header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), TRNSP_VERSION + 1);
  otherGuidStr = boost::lexical_cast<std::string>(header.GetGuid());
  EXPECT_EQ(guidStr, otherGuidStr);
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetType(), ADV_SVC);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) + sizeof(header.GetGuid()) +
    sizeof(header.GetTopicLength()) + topic.size() + sizeof(header.GetType()) +
    sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  address = "inproc://local";
  advMsg.SetAddress(address);
  EXPECT_EQ(advMsg.GetAddress(), address);
}

//////////////////////////////////////////////////
TEST(PacketTest, AdvMsgIO)
{
  boost::uuids::uuid guid = boost::uuids::random_generator()();
  std::string topic = "topic_test";

  // Pack an AdvMsg
  transport::Header otherHeader(TRNSP_VERSION, guid, topic, ADV, 3);
  std::string address = "tcp://10.0.0.1:6000";
  transport::AdvMsg advMsg(otherHeader, address);
  char *buffer = new char[advMsg.GetMsgLength()];
  size_t bytes = advMsg.Pack(buffer);
  EXPECT_EQ(bytes, advMsg.GetMsgLength());

  // Unpack an AdvMsg
  transport::Header header;
  transport::AdvMsg otherAdvMsg;
  size_t headerBytes = header.Unpack(buffer);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvMsg.SetHeader(header);
  char *pBody = buffer + header.GetHeaderLength();
  size_t bodyBytes = otherAdvMsg.UnpackBody(pBody);
  delete[] buffer;

  // Check that after Pack() and Unpack() the data does not change
  EXPECT_EQ(otherAdvMsg.GetAddressLength(), advMsg.GetAddressLength());
  EXPECT_EQ(otherAdvMsg.GetAddress(), advMsg.GetAddress());
  EXPECT_EQ(otherAdvMsg.GetMsgLength(), advMsg.GetMsgLength());
  EXPECT_EQ(otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength(), advMsg.GetMsgLength() -
            advMsg.GetHeader().GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
