Simple library (with implementations in languages of interest) to do discovery
on top of zeromq messaging.  The intent is that this library would be
incorporated into other projects, where things like message serialization would
be added.

Raw message definitions:

  * Header (HDR):
    * VERSION: 2 bytes
    * GUID: 16 bytes; ID that is unique to the process, generated according
      to RFC 4122
    * TOPICLENGTH: 1 byte; length, in bytes, of TOPIC
    * TOPIC: string; max 192 bytes
    * TYPE: 1 byte
    * FLAGS: 16 bytes (unused for now)
    * (max total: 2+16+1+192+1+16 = 228)

  * advertisement (ADV) or advertisement of service (ADV\_SVC):
    * HDR (TYPE = 1)
    * ADDRESSLENGTH: 2 bytes; length, in bytes, of ADDRESSES
    * ADDRESS: one valid ZeroMQ address (e.g., "tcp://10.0.0.1:6000"); max
      length 6+255+6=267 bytes
    * (max total: 228+267=495)

  * subscription (SUB) or subscription to service (SUB\_SVC):
    * HDR (TYPE = 2)
    * (null body)

ZeroMQ message definitions (for which we will let zeromq handle framing):

  * publication (PUB) or service request (REQ) or service reply (REP\_OK 
    or REP\_ERR) are multipart messages, with the following parts:
    * HDR (TYPE = ?)
    * BODY: opaque bytes

Defaults and conventions:

  * Default port for SUB and ADV messages: 11312
  * By convention, SUB and ADV messages are sent to a broadcast address.
  * In raw messages, all integers are sent little-endian
  * Every topic ADV should be accompanied by an ADV\_SVC that advertises a
    service by the same name as the topic.  This service can be used for 
    polled topic REQ/REP interactions.


API sketch:

  * `(un)advertise(topic)`
  * `(un)subscribe(topic, cb)`
    * `cb(topic, msg)`
  * `publish(topic, msg)`
  * `(un)advertise_service(topic, cb)`
    * `rep <- cb(topic, req)`
  * `init_service(topic)`
  * `wait_for_service(topic, timeout)`
  * `rep <- call_service(topic, req, timeout)`
  * `call_service_async(topic, req, cb)`
    * `cb(topic, rep)`
