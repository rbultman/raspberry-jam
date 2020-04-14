#!/bin/bash
echo Killing Jack
sudo killall jacktrip
sudo killall jackd
sleep 1s
#sudo service ntp stop
export JACK_NO_AUDIO_RESERVATION=1
sudo service triggerhappy stop
sudo service dbus stop
#sudo killall console-kit-daemon
#sudo killall polkitd
## Only needed when Jack2 is compiled with D-Bus support
#export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/dbus/system_bus_socket
sudo mount -o remount,size=128M /dev/shm
#killall gvfsd
#killall dbus-daemon
#killall dbus-launch
## Uncomment if you'd like to disable the network adapter completely
#echo -n "1-1.1:1.0" | sudo tee /sys/bus/usb/drivers/smsc95xx/unbind
echo -n performance | sudo tee /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
jackd -P70 -p16 -t2000 -d alsa $1 $2 -s -S &
sleep 1s
jacktrip $3 -n 1 $4 $5 $6
exit
