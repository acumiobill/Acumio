#!/bin/sh

SERVERLOCATION=`which AcumioServer.exe`
if [ -z $SERVERLOCATION ];
  then echo "You need to have AcumioServer.exe in your path.";
  return 1;
fi

AcumioServer.exe `hostname`:1782
