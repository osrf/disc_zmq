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

#include <uuid/uuid.h>
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

#define GUID_STR_LEN (sizeof(uuid_t) * 2) + 4 + 1

static char *msgTypesStr[] = {
    NULL, (char*)"ADVERTISE", (char*)"SUBSCRIBE", (char*)"ADV_SRV",
    (char*)"SUB_SVC", (char*)"PUB", (char*)"REQ", (char*)"SRV_REP_OK",
    (char*)"SRV_REP_ERROR"
};

namespace transport
{
  /// \brief Get the string representation of the GUID.
  /// \param[in] _uuid UUID to be converted to string.
  /// \return A string representation of the GUID.
  std::string GetGuidStr(const uuid_t &_uuid);

  class Header
  {
    /// \brief Constructor.
    public: Header();

    /// \brief Constructor.
    /// \param[in] _version Version of the transport library.
    /// \param[in] _guid Global identifier. Every process has a unique guid.
    /// \param[in] _topic Topic
    /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
    /// \param[in] _flags Optional flags that you want to include in the header.
    public: Header(const uint16_t _version,
                   const uuid_t &_guid,
                   const std::string &_topic,
                   const uint8_t _type,
                   const uint16_t _flags);

    /// \brief Get the transport library version.
    /// \return Transport library version.
    public: uint16_t GetVersion() const;

    /// \brief Get the guid.
    /// \return A unique global identifier for every process.
    public: uuid_t& GetGuid();

    /// \brief Get the topic length.
    /// \return Topic length in bytes.
    public: uint16_t GetTopicLength() const;

    /// \brief Get the topic.
    /// \return Topic name.
    public: std::string GetTopic() const;

    /// \brief Get the message type.
    /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
    public: uint8_t GetType() const;

    /// \brief Get the message flags.
    /// \return Message flags used for compression or other optional features.
    public: uint16_t GetFlags() const;

    /// \brief Set the transport library version.
    /// \param[in] Transport library version.
    public: void SetVersion(const uint16_t _version);

    /// \brief Set the guid.
    /// \param[in] _guid A unique global identifier for every process.
    public: void SetGuid(const uuid_t &_guid);

    /// \brief Set the topic.
    /// \param[in] _topic Topic name.
    public: void SetTopic(const std::string &_topic);

    /// \brief Set the message type.
    /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
    public: void SetType(const uint8_t _type);

    /// \brief Set the message flags.
    /// \param[in] _flags Used for enable optional features.
    public: void SetFlags(const uint16_t _flags);

    /// \brief Get the header length.
    /// \return The header length in bytes.
    public: int GetHeaderLength();

    /// \brief Print the header.
    public: void Print();

    /// \brief Serialize the header. The caller has ownership of the
    /// buffer and is responsible for its [de]allocation.
    /// \param[out] _buffer Destination buffer in which the header
    /// will be serialized.
    /// \return Number of bytes serialized.
    public: size_t Pack(char *_buffer);

    /// \brief Unserialize the header.
    /// \param[in] _buffer Input buffer containing the data to be unserialized.
    public: size_t Unpack(const char *_buffer);


    /// \brief Calculate the header length.
    private: void UpdateHeaderLength();

    /// \brief Version of the transport library.
    private: uint16_t version;

    /// \brief Global identifier. Every process has a unique guid.
    private: uuid_t guid;

    /// \brief Topic length in bytes.
    private: uint16_t topicLength;

    /// \brief Topic.
    private:std::string topic;

    /// \brief Message type (ADVERTISE, SUBSCRIPTION, ...).
    private: uint8_t type;

    /// \brief Optional flags that you want to include in the header.
    private: uint16_t flags;

    /// \brief Header length.
    private: int headerLength;
  };

  class AdvMsg
  {

    /// \brief Constructor.
    public: AdvMsg();

    /// \brief Constructor.
    /// \param[in] _header Message header
    /// \param[in] _address ZeroMQ valid address (e.g., "tcp://10.0.0.1:6000").
    public: AdvMsg(const Header &_header, const std::string &_address);

    /// \brief Get the message header.
    /// \return Reference to the message header.
    public: Header& GetHeader();

    /// \brief Get the address length.
    /// \brief Return the ZMQ address length (num of bytes).
    public: uint16_t GetAddressLength() const;

    /// \brief Get the ZMQ address.
    /// \return Return the ZMQ address.
    public: std::string GetAddress() const;

    /// \brief Set the header of the message.
    /// \param[in] _header Message header.
    public: void SetHeader(const Header &_header);

    /// \brief Set the ZMQ address.
    /// \param[in] _address ZMQ address to be contained in the message.
    public: void SetAddress(const std::string &_address);

    /// \brief Get the total length of the message.
    /// \return Return the length of the message in bytes.
    public: size_t GetMsgLength();

    /// \brief Print the message.
    public: void PrintBody();

    /// \brief Serialize the AdvMsg.
    /// \param[out] _buffer Buffer where the message will be serialized.
    /// \return The length of the serialized message in bytes.
    public: size_t Pack(char *_buffer);

    /// \brief Unserialize a stream of bytes into a AdvMsg.
    /// \param[out] _buffer Unpack the body from the buffer.
    /// \return The number of bytes from the body.
    public: size_t UnpackBody(char *_buffer);

    /// \brief Update the ADV message length.
    private: void UpdateMsgLength();

    /// \brief Message header
    private: Header header;

    /// \brief Length of the address contained in this message (bytes).
    private: uint16_t addressLength;

    /// \brief ZMQ valid address (e.g., "tcp://10.0.0.1:6000").
    private: std::string address;

    /// \brief Length of the message in bytes.
    private: int msgLength;
  };
}

#endif
