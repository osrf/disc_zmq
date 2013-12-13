Simple library (with implementations in languages of interest) to do discovery
on top of zeromq messaging.  The intent is that this library would be
incorporated into other projects, where things like message serialization would
be added.

Raw message definitions (for which we will manually pack/unpack):

  * Header (HDR):
    * OP: 1 byte
    * LENGTH: 2 bytes; length, in bytes, of the body to follow

  * advertisement (ADV):
    * HDR (OP = 1)
    * TOPICLENGTH: 2 bytes; length, in bytes, of TOPIC
    * TOPIC: string
    * GUID: 16 bytes; ID that is unique to the process, generated according to 
      RFC 4122
    * ADDRESSESLENGTH: 2 bytes; length, in bytes, of ADDRESSES
    * ADDRESSES: space-separated string of zeromq endpoint URIs

  * subscription (SUB):
    * HDR (OP = 2)
    * TOPICLENGTH: 2 bytes; length, in bytes, of TOPIC
    * TOPIC: string

zeromq message definitions (for which we will let zeromq handle framing):

  * data (DAT):
    * SENDER: space-separated string of zeromq endpoint URIs
    * TOPIC: string
    * MESSAGE: opaque bytes

Ports and addresses:

  * Default port for SUB and ADV messages: 11312
  * By convention, SUB and ADV messages are sent to a broadcast address.
