#!/usr/bin/env python
"""This is generating a file with git revision infos 
to be implemented in a python script"""
import os

dest="."+os.sep+"revision.py"
f=open(dest,"w")
f.write("hash=")
f.close()
os.system("git rev-parse --short HEAD >> "+dest)
