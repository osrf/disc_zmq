#ifndef __PACKET_HH_INCLUDED__
#define __PACKET_HH_INCLUDED__

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstddef>
#include <iostream>
#include <string.h>

//  This is the version of Gazebo transport we implement
#define TRNSP_VERSION       1

// Message types
#define ADV									1
#define SUB     						2
#define ADV_SVC							3
#define SUB_SVC  						4
#define PUB									5
#define REQ									6
#define REP_OK							7
#define REP_ERROR						8

static char *msgTypesStr [] = {
    NULL, (char*)"ADVERTISE", (char*)"SUBSCRIBE", (char*)"ADV_SRV",
    (char*)"SUB_SVC", (char*)"PUB", (char*)"REQ", (char*)"SRV_REP_OK",
    (char*)"SRV_REP_ERROR"
};

class Header
{
	public:

	//  ---------------------------------------------------------------------
  /// \brief Constructor.
	Header ()
	  : headerLength(0)
	{
	}

	//  ---------------------------------------------------------------------
  /// \brief Constructor.
	/// \param[in] _version Version of the transport library.
	/// \param[in] _guid Global identifier. Every process has a unique guid.
	/// \param[in] _topicLength Topic length in bytes.
	/// \param[in] _topic Topic
	/// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
	/// \param[in] _flags Optional flags that you want to include in the header.
	Header (const uint16_t _version,
          const boost::uuids::uuid &_guid,
          const std::string &_topic,
          const uint8_t _type,
          const uint16_t _flags)
	{
		this->SetVersion(_version);
		this->SetGuid(_guid);
		this->SetTopic(_topic);
		this->SetType(_type);
		this->SetFlags(_flags);
		this->UpdateHeaderLength();
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the transport library version.
  /// \return Transport library version.
	uint16_t GetVersion() const
	{
		return this->version;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the guid.
  /// \return A unique global identifier for every process.
	boost::uuids::uuid GetGuid() const
	{
		return this->guid;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the topic length.
  /// \return Topic length in bytes.
	uint16_t GetTopicLength() const
	{
		return this->topicLength;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the topic.
  /// \return Topic name.
	std::string GetTopic() const
	{
		return this->topic;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the message type.
  /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
	uint8_t GetType() const
	{
		return this->type;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the message flags.
  /// \return The message flags used for compression or other optional features.
	uint16_t GetFlags() const
	{
		return this->flags;
	}

		//  ---------------------------------------------------------------------
  /// \brief Set the transport library version.
  /// \param[in] Transport library version.
	void SetVersion(const uint16_t _version)
	{
		this->version = _version;
	}

	//  ---------------------------------------------------------------------
  /// \brief Set the guid.
  /// \param[in] _guid A unique global identifier for every process.
	void SetGuid(const boost::uuids::uuid &_guid)
	{
		this->guid = _guid;
	}

	//  ---------------------------------------------------------------------
  /// \brief Set the topic.
  /// \param[in] _topic Topic name.
	void SetTopic(const std::string &_topic)
	{
		this->topic = _topic;
		this->topicLength = this->topic.size();
		this->UpdateHeaderLength();
	}

	//  ---------------------------------------------------------------------
  /// \brief Set the message type.
  /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
	void SetType(const uint8_t _type)
	{
		this->type = _type;
	}

	//  ---------------------------------------------------------------------
  /// \brief Set the message flags.
  /// \param[in] _flags Used for enable compression or other optional features.
	void SetFlags(const uint16_t _flags)
	{
		this->flags = _flags;
	}

	//  ---------------------------------------------------------------------
  /// \brief Get the header length.
  /// \return The header length in bytes.
	int GetHeaderLength()
	{
		return this->headerLength;
	}

  //  ---------------------------------------------------------------------
  /// \brief Print the header.
  void Print()
  {
    std::cout << "\t--------------------------------------\n";
    std::cout << "\tHeader:" << std::endl;
    std::cout << "\t\tVersion: " << this->GetVersion() << "\n";
    std::cout << "\t\tGUID: " << this->GetGuid() << "\n";
    std::cout << "\t\tTopic length: " << this->GetTopicLength() << "\n";
    std::cout << "\t\tTopic: [" << this->GetTopic() << "]\n";
    std::cout << "\t\tType: " << msgTypesStr[this->GetType()] << "\n";
    std::cout << "\t\tFlags: " << this->GetFlags() << "\n";
  }

	//  ---------------------------------------------------------------------
  /// \brief Serialize the header. The caller has ownership of the
  /// buffer and is responsible for its [de]allocation.
  /// \param[out] _buffer Destination buffer in which the header
  /// will be serialized.
  /// \return Number of bytes serialized.
	size_t Pack(char *_buffer)
	{
		if (this->headerLength == 0)
			return 0;

    memcpy(_buffer, &this->version, sizeof(this->version));
    _buffer += sizeof(this->version);
    memcpy(_buffer, &this->guid, this->guid.size());
    _buffer += this->guid.size();
    memcpy(_buffer, &this->topicLength, sizeof(this->topicLength));
    _buffer += sizeof(this->topicLength);
    memcpy(_buffer, this->topic.data(), this->topicLength);
    _buffer += this->topicLength;
    memcpy(_buffer, &this->type, sizeof(this->type));
    _buffer += sizeof(this->type);
    memcpy(_buffer, &this->flags, sizeof(this->flags));

    return this->headerLength;
	}

	//  ---------------------------------------------------------------------
  /// \brief Unserialize the header.
  /// \param[in] _buffer Input buffer containing the data to be unserialized.
	size_t Unpack(const char *_buffer)
	{
		// Read the version
    memcpy(&this->version, _buffer, sizeof(this->version));
    _buffer += sizeof(this->version);

    // Read the GUID
    memcpy(&this->guid, _buffer, this->guid.size());
    _buffer += this->guid.size();

    // Read the topic length
    memcpy(&this->topicLength, _buffer, sizeof(this->topicLength));
    _buffer += sizeof(this->topicLength);

    // Read the topic
    char *topic = new char[this->topicLength + 1];
    memcpy(topic, _buffer, this->topicLength);
    topic[this->topicLength] = '\0';
    this->topic = topic;
    _buffer += this->topicLength;
    delete[] topic;

    // Read the message type
    memcpy(&this->type, _buffer, sizeof(this->type));
    _buffer += sizeof(this->type);

    // Read the flags
    memcpy(&this->flags, _buffer, sizeof(this->flags));
    _buffer += sizeof(this->flags);

    this->UpdateHeaderLength();
    return this->GetHeaderLength();
	}

	private:

		//  ---------------------------------------------------------------------
  	/// \brief Calculate the header length.
		void UpdateHeaderLength()
		{
			this->headerLength = sizeof(this->version) + this->guid.size() +
            						   sizeof(this->topicLength) + this->topic.size() +
          	  					   sizeof(this->type) + sizeof(this->flags);
		}

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
		//  ---------------------------------------------------------------------
		/// \brief Constructor.
		AdvMsg()
		  :  msgLength(0)
		{
		}

		//  ---------------------------------------------------------------------
		/// \brief Constructor.
		/// \param[in] _header Message header
		/// \param[in] _address ZeroMQ valid address (e.g., "tcp://10.0.0.1:6000").
		AdvMsg(const Header &_header,
		       const std::string &_address)
		{
			this->SetHeader(_header);
			this->SetAddress(_address);
			this->UpdateMsgLength();
		}

		Header& GetHeader()
		{
			return this->header;
		}

		uint16_t GetAddressLength() const
		{
			return this->addressLength;
		}

		std::string GetAddress() const
		{
			return this->address;
		}

		void SetHeader(const Header &_header)
		{
			this->header = _header;
			if (_header.GetType() != ADV && _header.GetType() != ADV_SVC)
				std::cerr << "You're trying to use a "
				          << msgTypesStr[_header.GetType()] << " header inside an ADV"
				          << " or ADV_SVC. Are you sure you want to do this?\n";
		}

		void SetAddress(const std::string &_address)
		{
			this->address = _address;
			this->addressLength = this->address.size();
			this->UpdateMsgLength();
		}

		size_t GetMsgLength()
		{
			return this->header.GetHeaderLength() + sizeof(this->addressLength) +
    				 this->address.size();
		}

    void PrintBody()
    {
      std::cout << "\tBody:" << std::endl;
      std::cout << "\t\tAddr size: " << this->GetAddressLength() << std::endl;
      std::cout << "\t\tAddress: " << this->GetAddress() << std::endl;
    }

		size_t Pack(char *_buffer)
		{
			if (this->msgLength == 0)
			return 0;

			this->GetHeader().Pack(_buffer);
			_buffer += this->GetHeader().GetHeaderLength();

	    memcpy(_buffer, &this->addressLength, sizeof(this->addressLength));
      _buffer += sizeof(this->addressLength);
      memcpy(_buffer, this->address.data(), this->address.size());

    	return this->GetMsgLength();
		}

		size_t UnpackBody(char *_buffer)
		{
			// Read the address length
      memcpy(&this->addressLength, _buffer, sizeof(this->addressLength));
      _buffer += sizeof(this->addressLength);

      // Read the address
      char *address = new char[this->addressLength + 1];
      memcpy(address, _buffer, this->addressLength);
      address[this->addressLength] = '\0';
      this->address = address;

      this->UpdateMsgLength();

      return sizeof(this->addressLength) + this->address.size();
		}

	private:

		//  ---------------------------------------------------------------------
  	/// \brief Calculate the header length.
		void UpdateMsgLength()
		{
			this->msgLength = this->GetHeader().GetHeaderLength() +
			                  sizeof(this->addressLength) + this->address.size();
		}

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
