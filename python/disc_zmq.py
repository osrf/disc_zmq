# Copyright (C) 2013 Open Source Robotics Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import socket
import zmq
import uuid
import os
import struct
import threading
import random
import signal
import sys

# Defaults and overrides
ADV_SUB_PORT = 11312
ADV_SUB_HOST = '255.255.255.255'
DZMQ_PORT_KEY = 'DZMQ_BCAST_PORT'
DZMQ_HOST_KEY = 'DZMQ_BCAST_HOST'
DZMQ_IP_KEY = 'DZMQ_IP'

# Constants
OP_ADV = 0x01
OP_SUB = 0x02
UDP_MAX_SIZE = 512
GUID_LENGTH = 16
ADV_REPEAT_PERIOD = 1.0

# We want this called once per process
GUID = uuid.uuid1()

class DZMQ:
    """
    This class provides a basic pub/sub system with discovery.  Basic
    publisher:
import disc_zmq
d = disc_zmq.DZMQ()
d.advertise('foo')
msg = 'bar'
while True:
    d.publish('foo', msg)
    d.spinOnce(0.1)

    Basic subscriber:
from __future__ import print_function
import disc_zmq
d = disc_zmq.DZMQ()
d.subscribe('foo', lambda topic,msg: print('Got %s on %s'%(topic,msg)))
d.spin()
    """
    def __init__(self, context=None):
        self.context = context or zmq.Context.instance()

        # Determine network addresses.  Look at environment variables, and 
        # fall back on defaults.

        # What's our broadcast port?
        if DZMQ_PORT_KEY in os.environ:
            self.bcast_port = int(os.environ[DZMQ_PORT_KEY])
        else:
            # Take the default
            self.bcast_port = ADV_SUB_PORT
        # What's our broadcast host?
        if DZMQ_HOST_KEY in os.environ:
            self.bcast_host = os.environ[DZMQ_HOST_KEY]
        else:
            # TODO: consider computing a more specific broadcast address based
            # on the result of get_local_addresses()
            self.bcast_host = ADV_SUB_HOST

        # What IP address will we give to others to use when contacting us?
        if DZMQ_IP_KEY in os.environ:
            self.ipaddr = os.environ[DZMQ_IP_KEY]
        else:
            # Try to find a non-loopback interface and then compute a broadcast
            # address from it.
            addrs = get_local_addresses()
            non_local_addrs = [x for x in addrs if not x.startswith('127')]
            if len(non_local_addrs) == 0:
                # Oh, well.
                self.ipaddr = addrs[0]
            else:
                # Take the first non-local one.
                self.ipaddr = non_local_addrs[0]

        # Set up to listen to broadcasts
        self.bcast_recv = socket.socket(socket.AF_INET, # Internet
                                        socket.SOCK_DGRAM) # UDP
        self.bcast_recv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.bcast_recv.bind((self.bcast_host, self.bcast_port))
        # Set up to send broadcasts
        self.bcast_send = socket.socket(socket.AF_INET, # Internet
                                        socket.SOCK_DGRAM) # UDP
        self.bcast_send.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        # Bookkeeping (which should be cleaned up)
        self.publishers = []
        self.subscribers = []
        self.sub_connections = []
        self.poller = zmq.Poller()
        self.poller.register(self.bcast_recv, zmq.POLLIN)
        # TODO: figure out what happens with multiple classes
        self.adv_timer = None
        self._advertisement_repeater()
        signal.signal(signal.SIGINT, self._sighandler)

    def _sighandler(self, sig, frame):
        self.adv_timer.cancel()
        sys.exit(0)

    def _advertise(self, publisher):
        """
        Internal method to pack and broadcast ADV message.
        """
        body = ''
        body += struct.pack('<H', len(publisher['topic']))
        body += publisher['topic']
        for i in range(0,GUID_LENGTH):
            body += struct.pack('<B', (GUID.int >> i*8) & 0xFF)
        addrs_string = ' '.join(publisher['addrs'])
        body += struct.pack('<H', len(addrs_string))
        body += addrs_string

        msg = ''
        msg += struct.pack('<BH', OP_ADV, len(body))
        msg += body
        self.bcast_send.sendto(msg, (self.bcast_host, self.bcast_port))

    def advertise(self, topic):
        """
        Advertise the given topic.  Do this before calling publish().
        """
        publisher = {}
        publisher['socket'] = self.context.socket(zmq.PUB)
        inproc_addr = 'inproc://%s'%(topic)
        publisher['socket'].bind(inproc_addr)
        tcp_addr = 'tcp://%s'%(self.ipaddr)
        tcp_port = publisher['socket'].bind_to_random_port(tcp_addr)
        tcp_addr += ':%d'%(tcp_port)
        publisher['addrs'] = [inproc_addr, tcp_addr]
        publisher['topic'] = topic
        self.publishers.append(publisher)
        self.poller.register(publisher['socket'], zmq.POLLIN)
        self._advertise(publisher)

    def _subscribe(self, subscriber):
        """
        Internal method to pack and broadcast SUB message.
        """
        body = ''
        offset = 0
        body += struct.pack('<H', len(subscriber['topic']))
        offset += 2
        body += subscriber['topic']
        offset += len(subscriber['topic'])

        msg = ''
        msg += struct.pack('<BH', OP_SUB, len(body))
        msg += body
        self.bcast_send.sendto(msg, (self.bcast_host, self.bcast_port))

    def subscribe(self, topic, cb):
        """
        Subscribe to the given topic.  Received messages will be passed to
        given the callback, which should have the signature: cb(topic, msg).
        """
        # Record what we're doing
        subscriber = {}
        subscriber['topic'] = topic
        subscriber['cb'] = cb
        self.subscribers.append(subscriber)
        self._subscribe(subscriber)

    def publish(self, topic, msg):
        """
        Publish the given message on the given topic.  You should have called
        advertise() on the topic first.
        """
        [p['socket'].send(msg) for p in self.publishers if p['topic'] == topic]

    def _handle_adv_sub(self, msg):
        """
        Internal method to handle receipt of SUB and ADV messages.
        """
        try:
            data, addr = msg
            #print('%s sent %s'%(addr, data))
            # Unpack the header, which is:
            #   OP: 1 byte
            #   LENGTH: 2 bytes; length, in bytes, of the body to follow
            offset = 0
            op = struct.unpack_from('<B', data, offset)[0]
            offset += 1
            length = struct.unpack_from('<H', data, offset)[0]
            offset += 2

            if op == OP_ADV:
                # Unpack the ADV body,  which is:
                #   TOPICLENGTH: 2 bytes; length, in bytes, of TOPIC
                #   TOPIC: string
                #   GUID: 16 bytes; ID that is unique to the process, generated according to RFC 4122
                #   ADDRESSESLENGTH: 2 bytes; length, in bytes, of ADDRESSES
                #   ADDRESSES: space-separated string of zeromq endpoint URIs
                adv = {}
                if len(data) != (length + 3):
                    print('Warning: message length mismatch (expected %d, but '
                          'received %d); ignoring.'%((length+3), len(data)))
                    return
                topiclength = struct.unpack_from('<H', data, offset)[0]
                offset += 2
                adv['topic'] = data[offset:offset+topiclength]
                offset += topiclength
                guid = 0
                for i in range(0,GUID_LENGTH):
                    guid += struct.unpack_from('<B', data, offset)[0] << 8*i
                    offset += 1
                adv['guid'] = uuid.UUID(int=guid) 
                addresseslength = struct.unpack_from('<H', data, offset)[0]
                offset += 2
                adv['addresses'] = data[offset:offset+addresseslength].split()
                offset += addresseslength
                #print('ADV: %s'%(adv))
                
                # Are we interested in this topic?
                if [s for s in self.subscribers if s['topic'] == adv['topic']]:
                    # Yes, we're interested; make a connection
                    self._connect_subscriber(adv)

            elif op == OP_SUB:
                # Unpack the SUB body,  which is:
                #   TOPICLENGTH: 2 bytes; length, in bytes, of TOPIC
                #   TOPIC: string
                sub = {}
                if len(data) != (length + 3):
                    print('Warning: message length mismatch (expected %d, but '
                          'received %d); ignoring.'%((length+3), len(data)))
                    return
                topiclength = struct.unpack_from('<H', data, offset)[0]
                offset += 2
                sub['topic'] = data[offset:offset+topiclength]
                offset += topiclength
                #print('SUB: %s'%(sub))

                # If we're publishing this topic, re-advertise it to allow the
                # new subscriber to find us.
                [self._advertise(p) for p in self.publishers if p['topic'] == sub['topic']]

            else:
                print('Warning: got unrecognized OP: %d'%(op))

        except Exception as e:
            print('Warning: exception while processing SUB or ADV message: %s'%(e))

    def _connect_subscriber(self, adv):
        """
        Internal method to connect to a publisher.
        """
        # Choose the best address to use.  If the publisher's GUID is the same
        # as our GUID, then we must both be in the same process, in which case
        # we'd like to use an 'inproc://' address.  Otherwise, fall back on
        # 'tcp://'.
        print('Addresses: %s'%(adv['addresses']))
        tcpaddrs = [a for a in adv['addresses'] if a.startswith('tcp')]
        inprocaddrs = [a for a in adv['addresses'] if a.startswith('inproc')]
        if adv['guid'] == GUID and inprocaddrs:
            addr = inprocaddrs[0]
        else:
            addr = tcpaddrs[0]
        # Check for existing connection to this guy; we only want one.
        if [c for c in self.sub_connections if c['addr'] == addr]:
            print('Not connecting again to %s'%(addr))
            return
        # Create a zmq socket
        sock = self.context.socket(zmq.SUB)
        conn = {}
        conn['socket'] = sock
        conn['topic'] = adv['topic']
        conn['addr'] = addr
        # Subscribe to all messages by specifying an empty filter string.
        # By default, a SUB socket filters out all messages.
        sock.setsockopt(zmq.SUBSCRIBE, '')
        self.sub_connections.append(conn)
        sock.connect(addr)
        self.poller.register(sock, zmq.POLLIN)
        print('Connected to %s for %s'%(addr, adv['topic']))

    def _advertisement_repeater(self):
        [self._advertise(p) for p in self.publishers]
        self.adv_timer = threading.Timer(ADV_REPEAT_PERIOD, self._advertisement_repeater)
        self.adv_timer.start()

    def spinOnce(self, timeout=-1):
        """
        Check once for incoming messages, invoking callbacks for received
        messages.  Wait for up to timeout seconds.  For no waiting, set
        timeout=0. To wait forever, set timeout=-1.
        """
        if timeout < 0:
            # zmq interprets timeout=None as infinite
            timeout = None
        else:
            # zmq wants the timeout in milliseconds
            timeout = int(timeout*1e3)
        # Look for sockets that are ready to read
        events = self.poller.poll(timeout)
        # Process the events
        for e in events:
            # Is it the broadcast socket, which we manage?
            if e[0] == self.bcast_recv.fileno():
                # Assume that we get the whole message in one go
                self._handle_adv_sub(self.bcast_recv.recvfrom(UDP_MAX_SIZE))
            else:
                # Must be a zmq socket
                sock = e[0]
                # Look up the connection associated with this socket
                conns = [c for c in self.sub_connections if c['socket'] == sock]
                if conns:
                    conn = conns[0]
                    topic = conn['topic']
                    # Get the message (assuming that we get it all in one read)
                    msg = sock.recv()
                    # Invoke all the callbacks registered for this topic.
                    [s['cb'](topic, msg) for s in self.subscribers if s['topic']
== topic]

    def spin(self):
        """
        Give control to the message event loop.
        """
        while True:
            self.spinOnce(0.01)

# Stolen from rosgraph
# https://github.com/ros/ros_comm/blob/hydro-devel/tools/rosgraph/src/rosgraph/network.py
# cache for performance reasons
_local_addrs = None
def get_local_addresses(use_ipv6=False):
    """
    :returns: known local addresses. Not affected by ROS_IP/ROS_HOSTNAME,
``[str]``
    """
    # cache address data as it can be slow to calculate
    global _local_addrs
    if _local_addrs is not None:
        return _local_addrs

    local_addrs = None
    import platform
    if platform.system() in ['Linux', 'FreeBSD']:
        # unix-only branch
        v4addrs = []
        v6addrs = []
        import netifaces
        for iface in netifaces.interfaces():
            try:
                ifaddrs = netifaces.ifaddresses(iface)
            except ValueError:
                # even if interfaces() returns an interface name
                # ifaddresses() might raise a ValueError
                # https://bugs.launchpad.net/ubuntu/+source/netifaces/+bug/753009
                continue
            if socket.AF_INET in ifaddrs:
                v4addrs.extend([addr['addr'] for addr in ifaddrs[socket.AF_INET]])
            if socket.AF_INET6 in ifaddrs:
                v6addrs.extend([addr['addr'] for addr in ifaddrs[socket.AF_INET6]])
        if use_ipv6:
            local_addrs = v6addrs + v4addrs
        else:
            local_addrs = v4addrs
    else:
        # cross-platform branch, can only resolve one address
        if use_ipv6:
            local_addrs = [host[4][0] for host in socket.getaddrinfo(socket.gethostname(), 0, 0, 0, socket.SOL_TCP)]
        else:
            local_addrs = [host[4][0] for host in socket.getaddrinfo(socket.gethostname(), 0, socket.AF_INET, 0, socket.SOL_TCP)]
    _local_addrs = local_addrs
    return local_addrs
