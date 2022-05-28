#!/bin/bash

clear

# make the target

if [ "$1" = "clean" ]; then
	make clean > /dev/null
fi

if [ "$2" = "build" ]; then
	export CPPFLAGS="-O2"
	make 2>&1 | ./colorize.sh
elif [ "$2" = "debug" ]; then
	export CPPFLAGS="-g -O0"
	make debug 2>&1 | ./colorize.sh
fi