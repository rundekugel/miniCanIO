#!/bin/bash

make/generateRevisionFile.py

if [ -e env ] ; then
	echo "using existing virtual environment."
else
  echo "create virtual environment..."
  python -m venv env
fi

source env/bin/activate


pip install -f requirements.txt

python -m pyinstaller /home/gaul1/p/can/miniCanIO/configapp/canIoConfig.py

echo done.

