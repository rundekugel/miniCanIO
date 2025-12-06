#!/usr/bin/env python

"""config structs of CAN-IO-Device
details: https://github.com/rundekugel/miniCanIO
"""

import sys, os, time
import can
import binascii as ba
import struct
import structhelper as sh
import json

try:
    import revision
except:
   class revision:
      hash = "0000000"

__revision__ = revision.hash
__version__ = "0.0.1a"

NUMBER_OF_MSG_FILTERS = 9
CONFIG_SIZE = 36 + 22*NUMBER_OF_MSG_FILTERS 

verifyTypes = ["==","!=",">","<","AND","XOR"]
switchTypes = ["ON","OFF","TOGGLE","BLINK","TIME","PWM","FREQ"]

usekeys = ("objId", "canid","ext","bytepos","bitmask","verifyType","verifyValue","switchType","pin" )
valueashex = ("canid","bitmask", "rxid", "txid" )
                    
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
   structstring = sh.LITTLE_ENDIAN +sh.U32 +6*sh.U8
   count = 0
   _allFilters = []

   def __init__(self, data = None, objId = None):
      if 0: # objId==None:
         self.objId = filterconfig.count
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
   
   def parseFromDict(self, filterdict):
      cfg = dict()
      for k in filterdict:
         v=filterdict[k]
         if k in usekeys:
            if isinstance(v, str):
               if k in valueashex:
                  v = int(v, 0)
               if k == "verifyType":
                  v=verifyTypes.index(v)
               if k == "switchType":
                  v= switchTypes.index(v)                  
         cfg[k] = v
      self.__dict__ = cfg
      return filter
      

   def getFilterByObjId(id: int)->object: 
      for filter in filterconfig._allFilters:
         if filter.objId == id:
            return filter
      return None
      
   def getAsBytes(self)->bytes:
      if self.canid==None:
         return b""
      msgid = self.canid | ([0,1<<31][self.ext>0])
      data = struct.pack(self.structstring, msgid, self.bytepos, self.bitmask, 
                         self.verifyType, self.verifyValue, self.switchType, self.pin)
      return data
   
   def toString(self) -> str:
      text = os.linesep +f"Filter id: {self.objId}"+os.linesep
      try:
         if self.canid==None:
            text +="Empty"+os.linesep
            return text
         text += f"CAN id: {hex(self.canid)}"+os.linesep
         text += f"msg id is extendeds: {self.ext}"+os.linesep
         text += f"byte pos: {self.bytepos}"+os.linesep
         text += f"bit mask: {self.bitmask}=bin:{bin(self.bitmask)}"+os.linesep
         text += f"verify type: {self.verifyType}: {verifyTypes[self.verifyType]}"+os.linesep
         text += f"verify value: {self.verifyValue}"+os.linesep
         text += f"switch type: {self.switchType}: {switchTypes[self.switchType]}"+os.linesep
         text += f"output pin: {self.pin}"+os.linesep
      except Exception as e:
        text += os.linesep+str(e) 
      return text
   
   def asJson(self, asHex=True)->str:
      usekeys = ("objId", "canid","ext","bytepos","bitmask","verifyType","verifyValue","switchType","pin" )
      #valueashex = ("canid","bitmask" )
      cfg =dict()
      for k in usekeys:
         v = self.__dict__.get(k)
         if asHex and k in valueashex:
            v = hex(v)
         if k == "verifyType":
            v=verifyTypes[self.verifyType]
         if k == "switchType":
            v= switchTypes[self.switchType]

         cfg[k] = v
      cfgjsn = json.dumps(cfg)
      return cfgjsn


   def asDict(self, asHex=True)->str:
      #usekeys = ("objId", "canid","ext","bytepos","bitmask","verifyType","verifyValue","switchType","pin" )
      #valueashex = ("canid","bitmask" )
      cfg =dict()
      for k in usekeys:
         v = self.__dict__.get(k)
         cfg[k] = v
      return cfg

   def addFilterByData(data, objId=None):
      _ = filterconfig(data, objId)

   def getAllFiltersAsBytes(filters=None):
      if filters==None:
          filters = filterconfig._allFilters
      data = b"\0"*(filterconfig.size * NUMBER_OF_MSG_FILTERS)
      for f in filters:
         if f.canid != 0:
            ld = data[:f.objId*f.size]
            rd = data[f.objId*f.size+filterconfig.size:]
            data = ld + f.getAsBytes() +rd
      return data
   

class config:
   size = 40
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
   filters = []
   dbgValues = None
   structstring = sh.LITTLE_ENDIAN +sh.U32 +"16"+sh.STRING +2*sh.U16 +4*sh.U32 # +9*filterconfig.structstring
   
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
         pos=self.size + i*filterconfig.size
         # f.append(filterconfig(data[pos:pos+filterconfig.size]))
         filterconfig(data[pos:pos+filterconfig.size], i)
      self.filters = filterconfig._allFilters
      return self

   def parseFile(self, filename):
      usekeys = ("canspeed","rxid","txid","pinResetState","ack","noRetransmission",
                 "wakeup","extendedIds","filtersAreList", "filters")
      valueashex = ("rxid","txid","pinResetState", "canid","bytemask","bitmask","verifyValue") # or bin
      try:
         f = open(filename, "r")
         jscfg = json.load(f)
         f.close()
      except Exception as e:
         print("error:",e)
         return None
      for k in jscfg:
         if k in usekeys:
            v = jscfg[k]
            if k in valueashex:
               if isinstance(v, str):
                  self.__dict__[k] = int(v, 0)
                  continue
            self.__dict__[k] = v
         filters = self.filters
      self.filters=[]
      for f in filters:
         f0=filterconfig()
         f0.parseFromDict(f)

         self.filters.append(f0)
         if 0:
            for k in f:
               if k in valueashex:
                  v = f[k]
                  if isinstance(v, str):
                     v=int(v, 0)
                     f[k] = v

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
      data += filterconfig.getAllFiltersAsBytes(me.filters)
      return data
   
   def asTextSmall(me)->str:
      text = f"valid: {hex(me.valid)}"+os.linesep
      text += f"key: {ba.hexlify(me.key)}"+os.linesep
      text += f"version: 0x{hex(me.version)}"+os.linesep
      text += f"canSpeed: {me.canspeed_k} kb/s"+os.linesep
      text += f"configMsgIdRx: 0x{hex(me.rxid)}"+os.linesep
      text += f"configMsgIdTx: 0x{hex(me.txid)}"+os.linesep
      text += f"pinResetState: 0x{hex(me.pinResetState)}"+os.linesep
      text += f"ack: {me.ack}"+os.linesep
      text += f"noRetransmission: {me.noRetransmission}"+os.linesep
      text += f"wakeup: {me.wakeup}"+os.linesep
      text += f"extendedIds: {me.extendedIds}"+os.linesep
      text += f"filtersAreList: {me.filtersAreList}"+os.linesep
      text += f"Number of filters: {len(filterconfig._allFilters)}"+os.linesep
      return text
   
   def toString(me)->str:
      text = me.asTextSmall()
      for f in me.filters:
         text += f.toString()
      return text 
   
   def asJson(me, asHex=True)->str:
      usekeys = ("canspeed","rxid","txid","pinResetState","ack","noRetransmission","wakeup","extendedIds","filtersAreList")
      valueashex = ("rxid","txid","pinResetState")
      cfg =dict()
      for k in usekeys:
         v = me.__dict__.get(k)
         if asHex and k in valueashex:
            if isinstance(v, int):
               v = hex(v)
         cfg[k] = v
      filters = []
      cfg["filters"]=[]
      for f in me.filters:
         filters.append(f)
         cfg["filters"].append(f.asDict())
      cfgjsn = json.dumps(cfg)
      return cfgjsn
   

#eof
