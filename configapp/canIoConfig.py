#!/usr/bin/env python

"""Read/write config of CAN-IO-Device
details: https://github.com/rundekugel/miniCanIO

-h    help
-cfg=<config-file>
-can=<can-interface>  default: can0
-up   upload config to canIoDevice
-key=<secretkey>  use this key for unlock the device
-read[=configfile]    download config from miniCanIo device. optional write to config-file
-v=<0..9>  set verbosity level. default = 1

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
   writeconfigblock = 0xc2
   saveconfig = 0xc9

class globs:
  verbosity = 1
  interface = 'socketcan'
  channel = 'can0'  
  bus : can.BusABC
  is_extended_id = False
  msgIdRx = 0xb9
  msgIdTx = 0xbb
  rxconfig = [0]*CONFIG_SIZE
  lockstatus = 99
  lastCanMsg = can.Message()
  config = None
  configfile=None
  key = []
  cmd =[]


def cansend(msg, waitafter=10e-3):
   if isinstance(msg, (tuple,list)):
      msg=bytes(msg)
   elif isinstance(msg, int):
      msg = bytes((msg,))
   if len(msg)<8:
      msg+=b"\0"*(8-len(msg))
   msg = can.Message(arbitration_id=globs.msgIdTx, data=msg, is_extended_id=globs.is_extended_id)
   globs.bus.send(msg)
   time.sleep(waitafter)

def handle_c0(m: can.Message):
    globs.lockstatus = m.data[1]
    return

def handle_c2(m: can.Message):
    if globs.verbosity:
       print("rx:",hex(m.arbitration_id),ba.hexlify(m.data))
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
   msgs={"Locklevel (0=unlocked)":[cfgmsg.unlock,handle_c0], 
         "block data":[cfgmsg.writeconfigblock ,handle_c2],
         "Config data":[cfgmsg.readconfig,handle_cf]}
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
   time.sleep(3)
   globs.config.decode(globs.rxconfig)

def unlock(cfg:config):
   cansend(bytes((cfgmsg.unlock,))+ b"reset")
   for i in range(3):
      if globs.lockstatus == 0:
         return
      cansend(bytes((cfgmsg.unlock,))+ cfg.key[i:i+6])
      time.sleep(0.02)


def writeAllConfig(cfg):
   """write config to device"""
   packsize = 5  # for editconfig =4; writeconfigblock =5
   cfg.valid = CONFIG_VALID_DEFAULT
   data = cfg.asBytesWithFilters()
   # unlock
   unlock(globs.config)

   # wirte packets
   size = len(data)
   pos = 0
   block=0
   while pos < size:
      payload = data[pos:pos+packsize]
      cansend(bytes([cfgmsg.writeconfigblock, block])+ payload )
      block +=1
      pos += packsize
      time.sleep(0.02)

   # writeToFlash
   cansend((cfgmsg.saveconfig))
   return 


def writeOneFilter(filter : filterconfig):
   """write one filter to device"""
   data = filter.asBytes()
   id = filter.objId
   # unlock
   # wirte packets
   
   return 

def main():
  print("CAN-IO-Configurator V"+__version__) 

  filters = [
    # {"can_id": 0xbb, "can_mask": 0x7FF, "extended": False},
    {"can_id": globs.msgIdRx, "can_mask": 0x7ff, "extended": False},
  ]

  param = "", ""

  globs.config = config();


  for p in sys.argv:
      if "=" in p:
         p0,p1=p.split("=",1)
      else:
         p0,p1 = p,None

      if p0 =="-v" : globs.verbosity = int(p1)
      if p0=="-rxid": globs.verbosity = int(p1, 0)
      if p0=="-txid": globs.verbosity = int(p1, 0)
      if p0=="-pin": globs.cmd.append("p") ;param =  p1
      if p0== "-read": globs.cmd.append("down"); param =p1
      if p0== "-can": globs.channel =p1
      if p0== "-cfg": globs.configfile = p1 ; globs.cmd.append("cfg")
      if p0== "-up": globs.cmd.append("up") 
      if p0=="-key": globs.key = p1; globs.config.key = bytes()
      if p0 in ("-h","?","-?","-help","--help"):
         print(__doc__)
         return 0
  globs.bus = can.Bus(channel=globs.channel, interface=globs.interface)
  globs.bus.filters = filters
  globs.notifier = can.Notifier(globs.bus, listeners=[can_rx_handler]) 
 
  if "down" in  globs.cmd:
      readAllConfig()
      if globs.verbosity:
         print(globs.config.toString())
      cfg = globs.config.asJson()
      if globs.verbosity:
         print(cfg)
      if param:
        with open(param, "w") as f:
         f.write(cfg)

  if "cfg" in globs.cmd:
     if globs.verbosity:
        print("parse config file "+p1+"...")
     globs.config.parseFile(p1)
     if globs.verbosity:
         print("done.")  
     if globs.verbosity:
         print(globs.config.asJson())

  if "up" in globs.cmd:
     if globs.verbosity:
        print("upload config...")
     writeAllConfig(globs.config)
     if globs.verbosity:
         print("done.")

  time.sleep(1)

  globs.notifier.remove_listener(can_rx_handler)
  print("listener removed.")
  print(globs)
  time.sleep(2)
  return 1

if __name__ == "__main__":
   sys.exit( main())

#eof
