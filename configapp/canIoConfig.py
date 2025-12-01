#!/usr/bin/env python

"""Read/write config of CAN-IO-Device
details: https://github.com/rundekugel/miniCanIO
"""

import sys, os, time
import can
import binascii as ba
import struct
import structhelper as sh

try:
    import revision
except:
   class revision:
      hash = "0000000"

__revision__ = revision.hash
__version__ = "0.0.1a"

class globs:
  verbosity = 1
  interface = 'socketcan'
  channel = 'vcan0'  
  bus : can.BusABC
  is_extended_id = False
  msgIdRx = 0xb9
  msgIdTx = 0xbb
  rxconfig = [0]*0x100
  lockstatus = 99

class filterconfig:
   size = 22
   id = None
   ext = False
   pin = None
   structstring = sh.U32 +6*sh.U8

   def __init__(self, data = None):
      if isinstance(data, (str, bytes, list)):
         self.decode(data)
   def decode(self, data):
      if isinstance(data,str):
         data=data.encode()
      if isinstance(data,list):
         data=bytes(data)
      self.id, self.bytepos, self.bitmask, self.verifyType, self.verifyValue, \
        self.switchType, self.outputPin = struct.unpack(self.structstring, data)
      return self
   
class config:
   size = 22
   valid = None
   key = []
   version = 0
   canspeed_k = 0
   rxid = 0
   txid = 0
   pinResetState = 0xffffFFFF
   boolean_combi = 0
   ack=0
   noRetransmission=0
   wakeup=0
   extendedIds=0
   filtersAreList=0
   filters = [filterconfig]
   dbgValues = None
   structstring = sh.U32 +"16"+sh.STRING +2*sh.U16 +4*sh.U32 +9*filterconfig.structstring
   
   def __init__(self, data = None):
      if isinstance(data, (str, bytes, list)):
         self.decode(data)
   def decode(self, data):
      if isinstance(data,str):
         data=data.encode()
      if isinstance(data,list):
         data=bytes(data)
      up = struct.unpack(self.structstring, data[:36])
      self.valid, self.key, self.version, self.canspeed_k, self.rxid, \
        self.txid, self.pinResetState , self.boolean_combi \
            = up
      bc = self.boolean_combi
      self.ack = bc & 1
      self.noRetransmission = (bc & 2) >0
      self.wakeup =(bc & 4)>0
      self.extendedIds =(bc & 8)>0
      self.filtersAreList =(bc & 16)>0
      
      f=[]
      for i in range(9):
         pos=i*filterconfig.size
         f.append(filterconfig(data[pos:pos+filterconfig.size]))
      
      return self

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
    globs.rxconfig[pos:pos+6] = m.data[2:]
    return

def can_rx_handler(m: can.Message):
   print("_h:",m)
   msgs={"Locklevel (0=unlocked)":[0xc0,handle_c0], "Config data":[0xcf,handle_cf]}
   for msg in msgs:
      
      if m.dlc>0 and msgs[msg][0] == m.data[0]:
         print("rx: "+msg+": "+ba.hexlify(m.data[1:], " ").decode())
         msgs[msg][1](m)  # call handler

def main():
  print("CAN-IO-Configurator V"+__version__) 

  filters = [
    # {"can_id": 0xbb, "can_mask": 0x7FF, "extended": False},
    {"can_id": globs.msgIdRx, "can_mask": 0x7ff, "extended": False},
  ]

  globs.bus = can.Bus(channel=globs.channel, interface=globs.interface)
  globs.bus.filters = filters
  globs.notifier = can.Notifier(globs.bus, listeners=[can_rx_handler]) 
  cansend([0xcf,0])
  # print(globs.bus.recv())
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
