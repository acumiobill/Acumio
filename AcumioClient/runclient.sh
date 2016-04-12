#!/bin/sh
#
# Author: Bill Province (bill@acumio.com)
# Copyright 2016 Acumio
#
# This is helpful if you just want to connect to a server on your local
# machine. Helpful for development, but not really useful in a real
# deployment.

EXELOCATION=`which AcumioCommandLine.exe`
if [ -z $EXELOCATION ];
  then echo "You need to have AcumioCommandLine.exe in your path.";
  return 1;
fi

AcumioCommandLine.exe `hostname`:1782
