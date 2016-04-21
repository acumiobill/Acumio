#!/bin/sh
#
# Author: Bill Province
# Copyright (C) 2016 Acumio
# This script scrapes the result of a test run and if the test was successful,
# it copies the test run file the the "verified" file. Both the input
# test run file and the verified file name are supplied as arguments.
#
# Basically, this script will copy the input file to the output file location,
# but only if the desired criteria for the input file is passed.
# If the run file does not pass the desired criteria, this generates an
# error message.

if [ $# -ne 2 ]; then
  echo "Usage: $0 input_test_run_file output_verified_file"
  exit 1
fi

lastline=`tail -1 $1`
passlocation=`expr match "$lastline" ".*PASSED"`
if [ $passlocation -gt 0 ]; then
  cp $1 $2
  exit 0
else
  echo "Test run $1 did not pass; will not create verify file"
  echo "Last line of run file: '$lastline'."
fi
