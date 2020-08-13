#!/bin/sh
killall blynk
killall jacktrip
killall jackd
xport JACK_NO_AUDIO_RESERVATION=1
sudo service triggerhappy stop
sudo service dbus stop
sudo mount -o remount,size=128M /dev/shm
echo -n performance | sudo tee /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
./blynk --token=`cat ../settings/blynk-token.txt` &

