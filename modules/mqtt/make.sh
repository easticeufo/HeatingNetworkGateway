#!/bin/bash

source ../../compile_config

if [ -z $1 ]; then
	${TOOL_PREFIX}gcc -pipe -Wall -O2 -c *.c
	${TOOL_PREFIX}ar rv libmqtt.a *.o
	cp libmqtt.a ../../libs/
elif [ $1 = "clean" ]; then
	rm -f *.o
	rm -f *.a
else
	echo "command error!"
fi
