#!/usr/bin/env python

import sys
import disc_zmq

if len(sys.argv) > 1:
    topic = sys.argv[1]
else:
    topic = 'foo'
if len(sys.argv) > 2:
    msg = sys.argv[2]
else:
    msg = 'foobar'

d = disc_zmq.DZMQ()
d.advertise(topic)

while True:
    #print('publishing %s on %s'%(msg, topic))
    d.publish(topic, msg)
    d.spinOnce(100)
