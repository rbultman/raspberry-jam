#!/bin/sh
killall blynk
killall jacktrip
killall jackd
./blynk --token=`cat ../settings/blynk-token.txt` &

