#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

make clean

set -e

if [ $DEBUG -eq 1 ]; then
  make CFLAGS=-DDEBUG
else
  make
fi

set +e
