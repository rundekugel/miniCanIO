#!/usr/bin/env python

"""emu read of config of CAN-IO-Device
details: https://github.com/rundekugel/miniCanIO
"""

import sys, os, time
import can
import binascii as ba
import struct
import structhelper as sh
import configStructs

try:
    import revision
except:
   class revision:
      hash = "0000000"

__revision__ = revision.hash
__version__ = "0.0.1a"

NUMBER_OF_MSG_FILTERS = 9
CONFIG_SIZE = 36 + 22*NUMBER_OF_MSG_FILTERS 

class cfgmsg:
   unlock = 0xc0
   readconfig = 0xcf
   editconfig = 0xce
   saveconfig = 0xc9

class globs:
  verbosity = 1
  interface = 'socketcan'
  channel = 'vcan0'  
  bus : can.BusABC
  is_extended_id = False
  msgIdRx = 0xbb
  msgIdTx = 0xb9
  rxconfig = [0]*CONFIG_SIZE
  lockstatus = 99
  lastCanMsg = can.Message()
  config = None

def cansend(msg):
   msg = can.Message(arbitration_id=globs.msgIdTx, data=msg, is_extended_id=globs.is_extended_id)
   globs.bus.send(msg)

def fillcfg():
   c = configStructs.config()
   c.ack = 1
   c.ack=1
   c.valid = 0xdeadbeef
   c.key = b"c"*16
   c.version = 1
   c.canspeed_k = 500 
   c.rxid = 0xbb
   c.txid = 0xb9
   c.pinResetState = 0xffffFFFF
   c.noRetransmission = 0
   c.wakeup = 1
   c.extendedIds=0
   c.filtersAreList=0
   f0=configStructs.filterconfig()
   f0.canid=0xbf
   f0.bytepos=2
   f0.pin=3
   f0.switchType=2
   f0.verifyValue=33
   f0.verifyType=1
   configStructs.filterconfig._allFilters.append(f0)
   c.filters=[f0]
   f1=configStructs.filterconfig()
   f1.__dict__=dict(f0.__dict__)
   f1.canid=0xbe
   f1.pin=4
   configStructs.filterconfig._allFilters.append(f1)
   c.filters=configStructs.filterconfig._allFilters
   globs.config = c
   
def main():
  print("CAN-IO-Config emu V"+__version__) 

  filters = [
    # {"can_id": 0xbb, "can_mask": 0x7FF, "extended": False},
    {"can_id": globs.msgIdRx, "can_mask": 0x7ff, "extended": False},
  ]

  globs.config = None;
  fillcfg()

  globs.bus = can.Bus(channel=globs.channel, interface=globs.interface)

  cfgAsData = globs.config.asBytesWithFilters()

  size = len(cfgAsData)
  block=0
  while size>block*6:
    time.sleep(.1)
    cansend(bytes([configStructs.cfgmsg.readconfig,block]) +cfgAsData[block*6:block*6+6])
    block +=1
  time.sleep(2)
  return 0

if __name__ == "__main__":
   sys.exit( main())

#eof
