tarname=rjam-deploy.tar
curdir=`pwd`

if [ -f "$tarname.gz" ]; then
	echo Removing old archive.
	rm $tarname.gz
fi
cd ..
echo Creating new arvhive.
echo Adding web server.
tar -cf $tarname raspberry-jam/rjam-web/
echo Adding settings.
tar --append -f $tarname raspberry-jam/settings/
echo Adding blynk program.
tar --append -f $tarname raspberry-jam/blynk-app/blynk
echo Adding run script.
tar --append -f $tarname raspberry-jam/blynk-app/run-blynk.sh
echo Adding jack script.
tar --append -f $tarname raspberry-jam/blynk-app/start_jack.sh
cd $curdir
echo Moving archive.
mv ../$tarname .
echo Compressing archive.
gzip $tarname
echo Done.
echo Archive is here: `pwd`/$tarname.gz

