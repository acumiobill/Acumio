#!/bin/sh
#
# Author: Bill Province
# Copyright (C) 2016 Acumio
# This script invokes AcumioServer.exe and starts it on this machine
# listening at port 1782.

SERVERLOCATION=`which AcumioServer.exe`
if [ -z $SERVERLOCATION ];
  then echo "You need to have AcumioServer.exe in your path.";
  return 1;
fi

AcumioServer.exe `hostname`:1782
