#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstddef>
#include <iostream>
#include <string.h>
#include "packet.hh"

//  ---------------------------------------------------------------------
Header::Header ()
  : headerLength(0)
{
}

//  ---------------------------------------------------------------------
Header::Header (const uint16_t _version,
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
uint16_t Header::GetVersion() const
{
	return this->version;
}

//  ---------------------------------------------------------------------
boost::uuids::uuid Header::GetGuid() const
{
	return this->guid;
}

//  ---------------------------------------------------------------------
uint16_t Header::GetTopicLength() const
{
	return this->topicLength;
}

//  ---------------------------------------------------------------------
std::string Header::GetTopic() const
{
	return this->topic;
}

//  ---------------------------------------------------------------------
uint8_t Header::GetType() const
{
	return this->type;
}

//  ---------------------------------------------------------------------
uint16_t Header::GetFlags() const
{
	return this->flags;
}

//  ---------------------------------------------------------------------
void Header::SetVersion(const uint16_t _version)
{
	this->version = _version;
}

//  ---------------------------------------------------------------------
void Header::SetGuid(const boost::uuids::uuid &_guid)
{
	this->guid = _guid;
}

//  ---------------------------------------------------------------------
void Header::SetTopic(const std::string &_topic)
{
	this->topic = _topic;
	this->topicLength = this->topic.size();
	this->UpdateHeaderLength();
}

//  ---------------------------------------------------------------------
void Header::SetType(const uint8_t _type)
{
	this->type = _type;
}

//  ---------------------------------------------------------------------
void Header::SetFlags(const uint16_t _flags)
{
	this->flags = _flags;
}

//  ---------------------------------------------------------------------
int Header::GetHeaderLength()
{
	return this->headerLength;
}

//  ---------------------------------------------------------------------
void Header::Print()
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
size_t Header::Pack(char *_buffer)
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
size_t Header::Unpack(const char *_buffer)
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

//  ---------------------------------------------------------------------
void Header::UpdateHeaderLength()
{
	this->headerLength = sizeof(this->version) + this->guid.size() +
        						   sizeof(this->topicLength) + this->topic.size() +
      	  					   sizeof(this->type) + sizeof(this->flags);
}

//  ---------------------------------------------------------------------
AdvMsg::AdvMsg()
  :  msgLength(0)
{
}

//  ---------------------------------------------------------------------
AdvMsg::AdvMsg(const Header &_header,
       const std::string &_address)
{
	this->SetHeader(_header);
	this->SetAddress(_address);
	this->UpdateMsgLength();
}

//  ---------------------------------------------------------------------
Header& AdvMsg::GetHeader()
{
	return this->header;
}

//  ---------------------------------------------------------------------
uint16_t AdvMsg::GetAddressLength() const
{
	return this->addressLength;
}

//  ---------------------------------------------------------------------
std::string AdvMsg::GetAddress() const
{
	return this->address;
}

//  ---------------------------------------------------------------------
void AdvMsg::SetHeader(const Header &_header)
{
	this->header = _header;
	if (_header.GetType() != ADV && _header.GetType() != ADV_SVC)
		std::cerr << "You're trying to use a "
		          << msgTypesStr[_header.GetType()] << " header inside an ADV"
		          << " or ADV_SVC. Are you sure you want to do this?\n";
}

//  ---------------------------------------------------------------------
void AdvMsg::SetAddress(const std::string &_address)
{
	this->address = _address;
	this->addressLength = this->address.size();
	this->UpdateMsgLength();
}

//  ---------------------------------------------------------------------
size_t AdvMsg::GetMsgLength()
{
	return this->header.GetHeaderLength() + sizeof(this->addressLength) +
				 this->address.size();
}

//  ---------------------------------------------------------------------
void AdvMsg::PrintBody()
{
  std::cout << "\tBody:" << std::endl;
  std::cout << "\t\tAddr size: " << this->GetAddressLength() << std::endl;
  std::cout << "\t\tAddress: " << this->GetAddress() << std::endl;
}

//  ---------------------------------------------------------------------
size_t AdvMsg::Pack(char *_buffer)
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

//  ---------------------------------------------------------------------
size_t AdvMsg::UnpackBody(char *_buffer)
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

//  ---------------------------------------------------------------------
void AdvMsg::UpdateMsgLength()
{
	this->msgLength = this->GetHeader().GetHeaderLength() +
	  sizeof(this->addressLength) + this->address.size();
}
