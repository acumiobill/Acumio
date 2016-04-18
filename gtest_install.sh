#!/bin/bash
#
# Author: Bill Province (bill@acumio.com)
# Copyright (C) 2016 Acumio
#
# This script is used to finish the installation of googletest and googlemock.
# Usage: gtest_install.sh
# (no arguments or parameters).
# This must be run from the directory that includes the googletest clone from
# github. See README for complete installation instructions. This script
# merely completes the installation process; it does not perform a complete
# end-to-end installation.

srcprefix="googletest/googletest/include/"
if [ ! -d ${srcprefix} ]; then
  echo "You must run this from a directory containing ${srcprefix} as a subdirectory."
  exit 1
fi

mockprefix="googletest/googlemock/include/"
if [ ! -d ${mockprefix} ]; then
  echo "You must run this from a directory containing ${mockprefix} as a subdirectory."
  exit 1
fi

libgmockloc="googletest/googlemock/libgmock.a"
libgmockmainloc="googletest/googlemock/libgmock_main.a"
if [[ ! -f $libgmockloc || ! -f $libgmockmainloc ]]; then
  echo "You must first run 'make' from googletest directory."
  echo "Also, if you have not yet invoked cmake CMakeLists.txt, you will"
  echo "need to also do that. See the README in our main github repository"
  echo "for detailed instructions."
  exit 1
fi

finallocation="/usr/include/"
if [ ! -w $finallocation ]; then
  echo "You must run this script using 'sudo'; i.e:"
  echo "sudo $0"
  exit 1
fi

# Invoked with input filename (should be either a .h or a .h.pump file)
# and output filename. The filenames should be relative to the current
# working directory.
gen_with_replace()
{
  sed 's/#\( *\)include\( *\)\"\(.*\)\"\(.*\)/#\1include\2\<\3\>\4/' $1 > $2
}

# Invoked with ls pattern (for use in an ls-listing), an output directory
# prefix, and a length parameter, indicating the string length of the
# directory where we are extracting files from.
# Example:
# gen_for_ls_pattern "googletest/googletest/include/gtest/internal/*.h" \
#                    "scratch/gtest/internal/" \
#                    45
gen_for_ls_pattern()
{
  for i in $( ls $1 ); do
    shortname_=${i:$3}
    out=$2/$shortname_
    gen_with_replace $i $out
  done
}

# Invoke as gen_for_each_suffix $srcprefix $outprefix $suffixlist
# For each provided file-name suffix (provided as a space-separated list),
# We perform a gen_for_ls_pattern with the ls pattern comprised of
# $srcprefix concatenated with "*" and then an individual suffix.
gen_for_each_suffix()
{
  srcprefixlen_=`expr length $1`
  for suffix_ in $3; do
    pattern_="$1*${suffix_}"
    gen_for_ls_pattern "$pattern_" "$2" "$srcprefixlen_"
  done
}

# Invoke as:
#  gen_for_each_directory_and_suffix $srcprefix \
#                                    $dstprefix \
#                                    $dirlist \
#                                    $suffixlist
# For each provided directory in dirlist, (which is a space-separated list
# of directories relative to both $srcprefix and $dstprefix),
# We perform a gen_for_each_suffix.
gen_for_each_directory_and_suffix()
{
  for dirname_ in $3; do
    srcprefix_=$1${dirname_}
    dstprefix_=$2${dirname_}
    gen_for_each_suffix "$srcprefix_" "$dstprefix_" "$4"
  done
}

rm -rf ./scratch
mkdir -p ./scratch/gtest/internal/custom
dstprefix="scratch/"
dirprefix=${srcprefix}"gtest/internal/custom/"
pattern=${dirprefix}"*.h"
outprefix=${dstprefix}"gtest/internal/custom/"
dirprefixlen=`expr length $dirprefix`
gen_for_ls_pattern "$pattern" "$outprefix" "$dirprefixlen"

dirnames="gtest/internal/ gtest/"
suffixes=".h .h.pump"
gen_for_each_directory_and_suffix "$srcprefix" "$dstprefix" "$dirnames" \
                                  "$suffixes"
rm -rf $finallocation/gtest
cp -a ./scratch/gtest $finallocation
rm -rf ./scratch

mkdir -p ./scratch/gmock/internal/custom
srcprefix="$mockprefix"
dirnames="gmock/internal/custom/ gmock/internal/ gmock/"
gen_for_each_directory_and_suffix "$srcprefix" "$dstprefix" "$dirnames" \
                                  "$suffixes"

rm -rf $finallocation/gmock
cp -a ./scratch/gmock $finallocation
rm -rf ./scratch
cp $libgmockloc /usr/lib
cp $libgmockmainloc /usr/lib
echo "Done."
