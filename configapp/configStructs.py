#!/usr/bin/env python

"""config structs of CAN-IO-Device
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

NUMBER_OF_MSG_FILTERS = 9
CONFIG_SIZE = 36 + 22*NUMBER_OF_MSG_FILTERS 

class cfgmsg:
   unlock = 0xc0
   readconfig = 0xcf
   editconfig = 0xce
   saveconfig = 0xc9

class filterconfig:
   objId = None
   size = 10
   canid = None
   bitmask = 0x0000
   ext = False
   pin = None
   structstring = sh.U32 +6*sh.U8
   count = 0
   _allFilters = []

   def __init__(self, data = None, objId = None):
      if 0: # objId==None:
         objId = filterconfig.count
         filterconfig.count+=1
      if isinstance(data, (str, bytes, list)):
         self.decode(data)
      else:
          self.addToAllFilters()

   def addToAllFilters(self):
       for f in filterconfig._allFilters:
           if f==self:
               return
       if self.objId==None:
          self.objId=filterconfig.count
       filterconfig._allFilters.append(self)
       filterconfig.count +=1

   def decode(self, data, objId=None):
      self.objId = objId
      if isinstance(data,str):
         data=data.encode()
      if isinstance(data,list):
         data=bytes(data)
      self.canid, self.bytepos, self.bitmask, self.verifyType, self.verifyValue, \
        self.switchType, self.pin = struct.unpack(self.structstring, data)
      o = filterconfig.getFilterByObjId(objId)
      if not o:
         if self.canid != 0 and self.canid!=0xffffFFFF:
            self.addToAllFilters()
      return self
   
   def getFilterByObjId(id: int): 
      for filter in filterconfig._allFilters:
         if filter.objId == id:
            return filter
      return None
      
   def getAsBytes(self):
      msgid = self.canid | ([0,1<<31][self.ext>0])
      data = struct.pack(self.structstring, msgid, self.bytepos, self.bitmask, 
                         self.verifyType, self.verifyValue, self.switchType, self.pin)
      return data
   
   def addFilterByData(data, objId=None):
      _ = filterconfig(data, objId)

   def getAllFiltersAsBytes(filters=None):
      if filters==None:
          filters = filterconfig._allFilters
      data = b"\0"*(filterconfig.size * NUMBER_OF_MSG_FILTERS)
      for f in filters:
         if f.canid != 0:
            ld = data[:f.objId]
            rd = data[f.objId+filterconfig.size:]
            data = ld + f.getAsBytes() +rd
      return data
   
class config:
   size = 22
   # configAsBytes = b"\0"*CONFIG_SIZE
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
   structstring = sh.U32 +"16"+sh.STRING +2*sh.U16 +4*sh.U32 # +9*filterconfig.structstring
   
   def __init__(self, data = None):
      if isinstance(data, (str, bytes, list)):
         self.decode(data)
   def decode(self, data):
      if isinstance(data,str):
         data=data.encode()
      if isinstance(data,list):
         data=bytes(data)
      up = struct.unpack(self.structstring, data[:40])
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
         # f.append(filterconfig(data[pos:pos+filterconfig.size]))
         filterconfig(data[pos:pos+filterconfig.size], i)
      return self

   def asBytesSmall(me, cfg=None) -> bytes:
      if cfg==None:
          cfg = me
      combo = [0,1][cfg.ack>0]
      combo |= [0,1<<1][cfg.noRetransmission>0]
      combo |= [0,1<<2][cfg.wakeup>0]
      combo |= [0,1<<3][cfg.extendedIds>0]
      combo |= [0,1<<4][cfg.filtersAreList>0]
      return struct.pack(cfg.structstring, cfg.valid, cfg.key, cfg.version, 
        cfg.canspeed_k, cfg.rxid, cfg.txid, cfg.pinResetState, combo)

   def asBytesWithFilters(me)-> bytes:
      data = me.asBytesSmall()
      data += filterconfig.getAllFiltersAsBytes()
      return data
#eof
