#!/usr/bin/env python

import sys
import disc_zmq

if len(sys.argv) > 1:
    topic = sys.argv[1]
else:
    topic = 'foo'

def cb(topic, msg):
    print('Got %s on %s'%(msg, topic))

d = disc_zmq.DZMQ()
d.subscribe(topic, cb)
d.spin()

