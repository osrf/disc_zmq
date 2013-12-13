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

# We want this called once per process
GUID = uuid.uuid1()

class DZMQ:

    def __init__(self, context=None):
        self.context = context or zmq.Context.instance()

        # Figure where we're broadcasting
        if DZMQ_PORT_KEY in os.environ:
            self.bcast_port = int(os.environ[DZMQ_PORT_KEY])
        else:
            # Take the default
            self.bcast_port = ADV_SUB_PORT
        if DZMQ_HOST_KEY in os.environ:
            self.bcast_host = os.environ[DZMQ_HOST_KEY]
        else:
            # TODO: consider computing a more specific broadcast address based
            # on the result of get_local_addresses()
            self.bcast_host = ADV_SUB_HOST
        # Figure out which of our addresses we'll give to others
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

        # Set up to listen
        self.bcast_recv = socket.socket(socket.AF_INET, # Internet
                                        socket.SOCK_DGRAM) # UDP
        self.bcast_recv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.bcast_recv.bind((self.bcast_host, self.bcast_port))
        # Set up to talk
        self.bcast_send = socket.socket(socket.AF_INET, # Internet
                                        socket.SOCK_DGRAM) # UDP
        self.bcast_send.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        self.publishers = []
        self.subscribers = []
        self.sub_connections = []
        self.poller = zmq.Poller()
        self.poller.register(self.bcast_recv, zmq.POLLIN)

    def _advertise(self, publisher):
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

    def subscribe(self, topic, cb):
        subscriber = {}
        subscriber['topic'] = topic
        subscriber['cb'] = cb
        self.subscribers.append(subscriber)

        body = ''
        offset = 0
        body += struct.pack('<H', len(topic))
        offset += 2
        body += topic
        offset += len(topic)

        msg = ''
        msg += struct.pack('<BH', OP_SUB, len(body))
        msg += body
        self.bcast_send.sendto(msg, (self.bcast_host, self.bcast_port))

    def publish(self, topic, msg):
        [p['socket'].send(msg) for p in self.publishers if p['topic'] == topic]

    def _handle_adv_sub(self, msg):
        try:
            data, addr = msg
            print('%s sent %s'%(addr, data))
            offset = 0
            op = struct.unpack_from('<B', data, offset)[0]
            offset += 1
            length = struct.unpack_from('<H', data, offset)[0]
            offset += 2

            if op == OP_ADV:
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
                print('ADV: %s'%(adv))
                subs = [s for s in self.subscribers if s['topic'] ==
adv['topic']]
                if subs:
                    self._connect_subscriber(adv)

            elif op == OP_SUB:
                sub = {}
                if len(data) != (length + 3):
                    print('Warning: message length mismatch (expected %d, but '
                          'received %d); ignoring.'%((length+3), len(data)))
                    return
                topiclength = struct.unpack_from('<H', data, offset)[0]
                offset += 2
                sub['topic'] = data[offset:offset+topiclength]
                offset += topiclength
                print('SUB: %s'%(sub))
                # Are we publishing on that topic?
                [self._advertise(p) for p in self.publishers if p['topic'] == sub['topic']]

            else:
                print('Got unrecognized OP: %d'%(op))

        except Exception as e:
            print('Warning: exception while processing SUB or ADV message: %s'%(e))

    def _connect_subscriber(self, adv):
        tcpaddrs = [a for a in adv['addresses'] if a.startswith('tcp')]
        inprocaddrs = [a for a in adv['addresses'] if a.startswith('inproc')]
        if adv['guid'] == GUID and inprocaddrs:
            addr = inprocaddrs[0]
        else:
            addr = tcpaddrs[0]
        # Check for existing connection to this guy
        if [c for c in self.sub_connections if c['addr'] == addr]:
            print('Not connecting again to %s'%(addr))
            return
        sock = self.context.socket(zmq.SUB)
        conn = {}
        conn['socket'] = sock
        conn['topic'] = adv['topic']
        conn['addr'] = addr
        # Subscribe to all messages
        sock.setsockopt(zmq.SUBSCRIBE, '')
        self.sub_connections.append(conn)
        sock.connect(addr)
        self.poller.register(sock, zmq.POLLIN)
        print('Connected to %s for %s'%(addr, adv['topic']))

    def spinOnce(self, timeout=0):
        events = self.poller.poll(int(timeout*1e3))
        for e in events:
            if e[0] == self.bcast_recv.fileno():
                # Assume that we get the whole message in one go
                self._handle_adv_sub(self.bcast_recv.recvfrom(UDP_MAX_SIZE))
            else:
                # Must be a zmq socket
                sock = e[0]
                conns = [c for c in self.sub_connections if c['socket'] == sock]
                if conns:
                    conn = conns[0]
                    topic = conn['topic']
                    msg = sock.recv()
                    [s['cb'](topic, msg) for s in self.subscribers if s['topic']
== topic]

    def spin(self):
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
