#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

make clean

if [ $DEBUG -eq 1 ]; then
  make CFLAGS=-DDEBUG
else
  make
fi


RESULT=$?
if [ $RESULT -ne 0 ]; then
  echo compile failed
  exit $?
fi
