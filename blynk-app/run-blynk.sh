#!/bin/sh
killall blynk
./blynk --token=`cat ../settings/blynk-token.txt` &

