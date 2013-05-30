#!/bin/bash
#
# Usage:
#   macosx_update_rpath.sh <dylib>
#
# Use install_name_tool(1) to update <dylib> so that the dynamic loader will
# look for VLC libraries in specific locations, and in a specific order.
#
# This script:
# - Changes the install names of the libvlc and libvlccore libraries so that
#   they refer to @rpath
# - Adds rpath commands, so that the dynamic loader will look for libvlc and
#   libvlccore under the specified locations

SCRIPT=$(basename "$0")
DYLIB="$1"

if [ ! -e "$DYLIB" ] ; then
  echo "[$SCRIPT] Error: file not found ('$1')" >&2
  exit 1
fi

doo(){
  echo "[$SCRIPT] $@"
  "$@"
}

# change_install_name oldname newname file.dylib
change_install_name(){
  OLD="$1"
  NEW="$2"
  FILE="$3"

  OTOOL_OUT=$(otool -l "$FILE")
  echo "$OTOOL_OUT" | grep -q "$OLD"
  if [ ! $? -eq 0 ] ; then
    # Old name not present
    echo "$OTOOL_OUT" | grep -q "$NEW"
    if [ ! $? -eq 0 ] ; then
      # New name not present => error
      echo "[$SCRIPT] Error: no reference to '$OLD' found in '$FILE'"
      exit 1
    fi
  else
    doo install_name_tool -change "$OLD" "$NEW" "$FILE"
  fi
}

# add_rpath rpath file.dylib
add_rpath() {
  RPATH="$1"
  FILE="$2"
  OTOOL_OUT=$(otool -l "$FILE")
  echo "$OTOOL_OUT" | grep -q "$RPATH"
  if [ ! $? -eq 0 ] ; then
    # rpath not present
    doo install_name_tool -add_rpath "$RPATH" "$FILE"
  fi
}

##

echo "[$SCRIPT] # Processing: '$DYLIB'"

change_install_name @loader_path/lib/libvlc.5.dylib @rpath/libvlc.5.dylib "$DYLIB"
change_install_name @loader_path/lib/libvlccore.5.dylib @rpath/libvlccore.5.dylib "$DYLIB"

# Paths where to look for VLC, * IN ORDER *
add_rpath @loader_path/vlc/lib "$DYLIB"
add_rpath @executable_path/../../../VLC.app/Contents/MacOS/lib "$DYLIB"
add_rpath /Applications/VLC.app/Contents/MacOS/lib "$DYLIB"

echo "[$SCRIPT] # '$DYLIB' is up-to-date"
