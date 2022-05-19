#!/bin/bash

echo $1
echo $2

for i in `seq 5`
do
    # Create the filename
    mosquitto_pub -h 192.168.1.1 -t "/mirror/$i/step_delay" -m $2
    mosquitto_pub -h 192.168.1.1 -t "/mirror/$i/position" -m $1
done

