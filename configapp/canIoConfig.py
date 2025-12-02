#!/usr/bin/env python

"""Read/write config of CAN-IO-Device
details: https://github.com/rundekugel/miniCanIO
"""

import sys, os, time
import can
import binascii as ba
import struct
import structhelper as sh
from configStructs import *

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
  msgIdRx = 0xb9
  msgIdTx = 0xbb
  rxconfig = [0]*CONFIG_SIZE
  lockstatus = 99
  lastCanMsg = can.Message()
  config = None


def cansend(msg):
   msg = can.Message(arbitration_id=globs.msgIdTx, data=msg, is_extended_id=globs.is_extended_id)
   globs.bus.send(msg)

def handle_c0(m: can.Message):
    globs.lockstatus = m.data[1]
    return

def handle_cf(m: can.Message):
    if m.dlc<8:
       print("Error: mesg too short")
       return
    pos = m.data[1]*6
    #data = m.data[2:]
    if globs.verbosity >1:
       print(f"fill cfg with pos: {pos}{ba.hexlify(m.data[2:],' ')}.")
    globs.rxconfig[pos:pos+6] = m.data[2:]
    return

def can_rx_handler(m: can.Message):
   print("_h:",m)
   msgs={"Locklevel (0=unlocked)":[0xc0,handle_c0], "Config data":[0xcf,handle_cf]}
   for msg in msgs:
      
      if m.dlc>0 and msgs[msg][0] == m.data[0]:
         print("rx: "+msg+": "+ba.hexlify(m.data[1:], " ").decode())
         msgs[msg][1](m)  # call handler
   globs.lastCanMsg = m

def readAllConfig():
   t0 = globs.lastCanMsg.timestamp
   timeout = 10
   for block in range(int(255/4)):
      cansend([cfgmsg.readconfig, block])
      end = time.time() + timeout
      while end > time.time():
         time.sleep(0.01)
         if t0 != globs.lastCanMsg.timestamp:
            break
   globs.config.decode(globs.rxconfig)



def main():
  print("CAN-IO-Configurator V"+__version__) 

  filters = [
    # {"can_id": 0xbb, "can_mask": 0x7FF, "extended": False},
    {"can_id": globs.msgIdRx, "can_mask": 0x7ff, "extended": False},
  ]

  globs.config = config();

  globs.bus = can.Bus(channel=globs.channel, interface=globs.interface)
  globs.bus.filters = filters
  globs.notifier = can.Notifier(globs.bus, listeners=[can_rx_handler]) 
  cansend([0xcf,0])
  # print(globs.bus.recv())
  readAllConfig()
  doit = 1
  while doit:
    time.sleep(1)
  globs.notifier.remove_listener(can_rx_handler)
  print("listener removed.")
  print(globs)
  time.sleep(2)
  return 1

if __name__ == "__main__":
   sys.exit( main())

#eof
