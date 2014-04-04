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

#ifndef __PACKET_HH_INCLUDED__
#define __PACKET_HH_INCLUDED__

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

//  This is the version of Gazebo transport we implement
#define TRNSP_VERSION       1

// Message types
#define ADV                 1
#define SUB                 2
#define ADV_SVC             3
#define SUB_SVC             4
#define PUB                 5
#define REQ                 6
#define REP_OK              7
#define REP_ERROR           8

static char *msgTypesStr[] = {
    NULL, (char*)"ADVERTISE", (char*)"SUBSCRIBE", (char*)"ADV_SRV",
    (char*)"SUB_SVC", (char*)"PUB", (char*)"REQ", (char*)"SRV_REP_OK",
    (char*)"SRV_REP_ERROR"
};

class Header
{
  public:
  /// \brief Constructor.
  Header();

  //  ---------------------------------------------------------------------
  /// \brief Constructor.
  /// \param[in] _version Version of the transport library.
  /// \param[in] _guid Global identifier. Every process has a unique guid.
  /// \param[in] _topicLength Topic length in bytes.
  /// \param[in] _topic Topic
  /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
  /// \param[in] _flags Optional flags that you want to include in the header.
  Header(const uint16_t _version,
         const boost::uuids::uuid &_guid,
         const std::string &_topic,
         const uint8_t _type,
         const uint16_t _flags);

  /// \brief Get the transport library version.
  /// \return Transport library version.
  uint16_t GetVersion() const;

  /// \brief Get the guid.
  /// \return A unique global identifier for every process.
  boost::uuids::uuid GetGuid() const;

  /// \brief Get the topic length.
  /// \return Topic length in bytes.
  uint16_t GetTopicLength() const;

  /// \brief Get the topic.
  /// \return Topic name.
  std::string GetTopic() const;

  /// \brief Get the message type.
  /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
  uint8_t GetType() const;

  /// \brief Get the message flags.
  /// \return The message flags used for compression or other optional features.
  uint16_t GetFlags() const;

  /// \brief Set the transport library version.
  /// \param[in] Transport library version.
  void SetVersion(const uint16_t _version);

  /// \brief Set the guid.
  /// \param[in] _guid A unique global identifier for every process.
  void SetGuid(const boost::uuids::uuid &_guid);

  /// \brief Set the topic.
  /// \param[in] _topic Topic name.
  void SetTopic(const std::string &_topic);

  /// \brief Set the message type.
  /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
  void SetType(const uint8_t _type);

  /// \brief Set the message flags.
  /// \param[in] _flags Used for enable compression or other optional features.
  void SetFlags(const uint16_t _flags);

  /// \brief Get the header length.
  /// \return The header length in bytes.
  int GetHeaderLength();

  /// \brief Print the header.
  void Print();

  /// \brief Serialize the header. The caller has ownership of the
  /// buffer and is responsible for its [de]allocation.
  /// \param[out] _buffer Destination buffer in which the header
  /// will be serialized.
  /// \return Number of bytes serialized.
  size_t Pack(char *_buffer);

  /// \brief Unserialize the header.
  /// \param[in] _buffer Input buffer containing the data to be unserialized.
  size_t Unpack(const char *_buffer);

  private:
    /// \brief Calculate the header length.
    void UpdateHeaderLength();

    /// \brief Version of the transport library.
    uint16_t version;

    /// \brief Global identifier. Every process has a unique guid.
    boost::uuids::uuid guid;

    /// \brief Topic length in bytes.
    uint16_t topicLength;

    /// \brief Topic.
    std::string topic;

    /// \brief Message type (ADVERTISE, SUBSCRIPTION, ...).
    uint8_t type;

    /// \brief Optional flags that you want to include in the header.
    uint16_t flags;

    /// \brief Header length.
    int headerLength;
};

class AdvMsg
{
  public:
    /// \brief Constructor.
    AdvMsg();

    /// \brief Constructor.
    /// \param[in] _header Message header
    /// \param[in] _address ZeroMQ valid address (e.g., "tcp://10.0.0.1:6000").
    AdvMsg(const Header &_header, const std::string &_address);

    /// \brief
    Header& GetHeader();

    /// \brief
    uint16_t GetAddressLength() const;

    /// \brief
    std::string GetAddress() const;

    /// \brief
    void SetHeader(const Header &_header);

    /// \brief
    void SetAddress(const std::string &_address);

    /// \brief
    size_t GetMsgLength();

    /// \brief
    void PrintBody();

    /// \brief
    size_t Pack(char *_buffer);

    /// \brief
    size_t UnpackBody(char *_buffer);

  private:
    /// \brief Calculate the header length.
    void UpdateMsgLength();

    /// \brief Message header
    Header header;

    /// \brief Length of the address contained in this message (bytes).
    uint16_t addressLength;

    /// \brief ZMQ valid address (e.g., "tcp://10.0.0.1:6000").
    std::string address;

    /// \brief Length of the message in bytes.
    int msgLength;
};

#endif
