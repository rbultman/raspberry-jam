### Blynk App
This program runs on a Raspberry Pi and can be used to control a jam session.

Details to come...

### Building
- Download the Blynk libraries
- Move the files to the pi
- Make
- Run

### Address Book
An example address book is included in this repo.  The format of the file is 
important.  The first line of the file is a header to illustrate how each
subsequent line in the file should be formatted.  The format is very simple:
```
username,ipaddress
```
There should be 4 users in the file.  If you won't have 4 users in your
address book, just include dummy lines.

When the progam starts, the users.csv file is read and it its contents are
used to populate the address book drop-down in the Blynk app.

