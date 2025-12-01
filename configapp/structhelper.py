"""
This module is intended to help building struct format strings.
use:

import structhelper as structh

formatstring = structh.LITTLE_ENDIAN +structh.U16 +structh.CHAR +structh.CHAR

bytes = struct.pack(formatstring,    65000, 9, -127)
a_uint16, b_char, c_char = struct.unpack(formatstring, bytes)
or
a_uint16, b_char, c_char = structh.unpack(formatstring, bytes)

"""
from struct import *
import struct as _struct
__DOC_STRUCT__ = _struct.__doc__

U8 = 'B'
UINT8 = U8
UCHAR = U8
S8 = 'b'
CHAR = S8
INT8 = S8

U16 = 'H'
UINT16 = U8
S16 = 'h'
INT16 = S16
U32 = 'I'
UINT32 = U32
S32 = 'i'
INT32 = S32
BOOL = '?'
PAD = 'x'   # no value, just padding byte
U64 = 'L'
ULONG = U64
S64 = 'l'
LONG = S64
U128 = 'Q'
U_LONG_LONG = U128
S128 = 'q'
LONG_LONG = S128
FLOAT = 'f'  #  IEEE 754 binary32
DOUBLE = 'd'  #  IEEE 754 binary64
FLOAT_HALF_PRECISION = 'e'  #  IEEE 754 binary16
BYTES_PASCAL = 's'
STRING = 's'
BYTES = 'p'

LITTLE_ENDIAN = '<'
BIG_ENDIAN = '>'

# eof
