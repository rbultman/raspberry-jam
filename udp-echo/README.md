### UDP Tools

This directory contains a few tools to help characterize the latency of UDP 
connections.  It consists of two programs,  a server and a client, that are 
meant to be used together in order to measure the latency of a network link.
Simply run the server on one computer and the client on the other.  The client
sends UDP messages to the server, and the server responds when a message is
recieved.  The client measures the elapsed time between sending the message
to the server and receiving the response.  It prints the measured time, the
minimum time, the maximum time, and the average time.  The average time is 
obtained using an infinite impulse response (IIR) filter.

- `udpserver` - This is the server.
- `udpclient` - This is the client.

This tool was meant to be as simple as possible in order to reduce latency 
effects caused by the program itself. A time measurement is started just
prior to sending the message to the server and is stopped as soon as
possible.  All printing and averaging is done outside of the measurement.
See the notes below for a means of determining the local latency and
removing that from the measurement.

Notes: 
1. UDP port 4464 needs to be open on the server in order for this to work.
2. This tool was compiled for and runs on Linux.  It might run on a Mac. 
I have no idea if it will run on a Windows computer.

### Building
```
cd ~/raspberry-jam/udp-echo
make
```

### Running
On the computer acting as a server: `./udpserver`

On the computer acting as a client: `./udpclient -s <server ip address> -n <some text>`

When a message is received on the server, <some text> will be printed.  
The text can be used to detect connections from multiple clients.  Several clients
could attempt to connect to the server simultaneously, so the messages printed at the 
server can be used to see if all clients are connecting.

### Example Output
Client Output:
```
pi@raspberry-jam:~/raspberry-jam/udp-echo $ ./udpclient -s x.yy.zzz.qqq -n Rob
Hello message 1 sent.
Server : Hello from server
Latency: 36.963, min: 36.963, max: 36.963, avg: 36.963
```

The latencies printed are in milliseconds.

Server Output:
```
server:~/raspberry-jam/udp-echo$ ./udpserver
Client : Rob
Hello message sent.
```

### Testing Local Latency
Some latency is due to the computers themselves and some to the network.
The client and server can be run on the same machine in an attempt to 
determine the local latency.

```
./udpserver &
./udpclient -s 127.0.0.1 -n local
```

