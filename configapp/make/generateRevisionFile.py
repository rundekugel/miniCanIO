#!/usr/bin/env python

import os

#os.system("git r")
f=open("."+os.sep+"revision.py","w")
f.write("hash=1234567\r\n")
f.close()
print("done.")
