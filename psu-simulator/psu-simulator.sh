#!/bin/bash

socat -d -d PTY,link=serialA PTY,link=serialB &

bg_pid=$!

sleep 0.1

clear
./psu-simulator $1 serialA

kill -15 $bg_pid

# reset the terminal console :-)
stty sane
clear