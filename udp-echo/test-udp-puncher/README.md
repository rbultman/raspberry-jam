## TEST CLIENT AND SERVER FOR UDP HOLE PUNCHER

_The test code is derived the code originally written by Rob Bultman github.com/rbultman for the Raspberry Jam project. Details at https://github.com/rbultman/raspberry-jam. This description was created by Vijay Vaidyanathan github.com/vvaidy and released into the Public Domain, Aug 1 2020._

### WARNINGS

1. This is a simple proof-of-concept NAT/UDP hole-puncher, and doesn't try to handle even the simple variations on the basic UDP hole-punching formula.
2. **SUPER IMPORTANT**. There is a **really good reason** that NAT server and UDP firewalls exist: SECURITY. Amongst other things, NAT protects your machine by making it inaccessible to other machines on the internet that might be trying to connect to your machine. This test hole-puncher code effectively denies your software and machine some of those protections. In fact, as implemented, it actually advertises and exposes your machine to external internet connections. So, PLEASE DO NOT DO THIS unless you really understand what you are doing. This code is intended purely for use by developers trying to understand how their NAT servers work. Please, DO NOT USE THIS CODE AS-IS FOR ANYTHING OTHER THAN LEARNING HOW THIS WORKS! YOU"VE BEEN WARNED, YOU ARE NOW ENTIRELY ON YOUR OWN. THERE ARE NO WARRANTIES OF ANY KIND etc etc etc.

OK, you may now read on :-)

### COMPILE

To compile (I've tested this on Ubuntu 18.04 LTS Bionic and MacOS 10.14.6 Mojave). All you need is a working `g++` (or equivalent).

```
g++ test-client.cpp -o test-client
g++ test-server.cpp -o test-server
```

### RUNNING THE SERVER

Run the test server. Make sure you are on a machine that is directly accessible on the open internet at the port you pick (e.g. by setting up port forwarding on your router or opening up ports on your firewall etc.):

```
./test-server -p PORT_NUMBER
```

e.g.

```
./test-server -p 8080
```

### RUNNING THE FIRST PEER TEST CLIENT

You need to run (at least) two test clients. They do NOT need to be directly accessible on the open internet, they only need to be able to directly access the internet). In a proper test, each test client should be behind a different firewall/NAT, but it will also work of both clients and the server are behind the same firewall/NAT:

```
./test-server -s TEST_SERVER_IP_ADDRESS -p TEST_SERVER_PORT_NUMBER -n A_NAME_I_PICK_FOR_MYSELF -c THE_NAME_OF_THE_PEER_I_AM_CONTACTING
```

To test the server and two clients on your local machine, start the server ...

```
./test-server -p 8080
```

Next, start a client called `alice` that wants to connect to a peer client called `bob`

```
./test-client -s 127.0.0.1 -p 8080 -n alice -c bob
```

The server should respond within a few seconds (configurable in the server code) with a "Phonebook" of entries for connected clients. Due to some artifacts of hot it works, it may take a few go around before you see the server response as such, but it should happen within about 30 seconds, depending how the server is configured. If this is the first client to connect you will only see a single entry in the phonebook at this time. See _How the server works_ below for more details.

### RUNNING THE SECOND PEER TEST CLIENT

Now start the `bob` peer client and tell it to connect to `alice`

```
./test-client -s 127.0.0.1 -p 8080 -n bob -c alice
```

You should get a response shortly with an updated phonebook that includes both the `bob` and `alice` entries. It might take about 10 or 20 seconds before you see the full phonebook (see the code for details). The `alice` peer client will also eventually refresh its request and see the new phonebook that will include bob. As soon as each peer client notices the entry they are trying to contact show up in the phonebook, they start trying to connect the peer client directly.

### How it works on the server

When two peers behind NATs want to connect to each other, there are two different problems that need to be solved. The first is that each peer is not directly addressable on the internet but is connecting through a NAT device of some sort that is assigning some new external address (including a port number) to the client socket. So, the first problem is to figure out what this external address is. The second problem is to signal to other people so they can reach you at this external address. The server handles these two problems through its phonebook.

Technologically speaking, problem #1 is easy enough to solve: there are a lot of ways a client can discover its own external IP address ... e.g. STUN or you can even just hit google or some other server that will tell you what _it_ saw as your IP address.

Problem #2 is trickier since you need some coordination mechnism to inform others about your availability and your externally accessible address and port. The test server solves problem #2 by requiring that when a peer client connects to the server, the client provides it with a unique handle - a unique name - that the client wants associated with itself. The server stores client names and client external addresses in its phonebook, and sends a copy of its phonebook as a response to anyone that connects to it.

The client name is pretty much any string (so make sure you pick a unique name for each test client!). The server phonebook is implemented as a simple static circular buffer, which means that entries are added one at a time as new clients connect. If the name is already in the phonebook, the old address is overwritten. If it's not in the address book, it's added. When MAXCLIENTS number of entries is reached, it just wraps around overwriting the oldest client entries. MAXCLIENTS is configurable in `test-server.cpp`. Each time a client connects to it, it sends back the entire phonebook as a response to that client. That's pretty much the only thing the server does.

_Note: In the code, I refer to the server as the "Stage Manager". The original analogy I was working towards was a piece of software that manages who is "on stage" and that all subscribers to a stage (think of it as a chat room for a digital stream) can receive direct, peer-to-peer streams from the entities "on stage". That analogy didnt quite survive, but the variable names did._

### How it works on each peer client

Each client is behind it's own NAT, and is not directly reachable on the internet. This means that no one "on the outside" can initiate connections to the client. However, here's the loophole on that rule: machines on the outside of the firewall _can_ send responses to packets that were initiated by the client, and these responses *are* forwarded back to the client. To make this reponse forwarding work, the NAT software sets up an exteral IP address and port mapping for server responses, and when received, these response packets are forwarded back to the client's initiating socket. Understanding how these responses are treated and forwarded back to the initiating client socket by the NAT software is crucial to understanding how UDP hole punching works.

Recall that when a client connects to the test server, it gets back the phone book of all other connected clients along with the IP addresses and ports for each client. Assume Alice and Bob are two clients that want to tallk directly to each other. Once Alice and Bob are connected to the test server, they each get a copy of the phonebook, and so can locate each other through each other's external IP addresses and port. However, that can't be used right away to connect to the client because the NAT will drop any packets that aren't a response to a connection initiated from within the NAT to that address.

Here is the crucial trick: both clients initiate connections to each other anyway! Let's assume Alice happens to go first. The first packet from Alice to Bob is in fact dropped by Bob's firewall because Bob hasn't (yet) initiated a connection to Alice. However, Bob's packet to Alice's external IP address and port _is_ sent forwarded back to Alice. Why? Because the NAT sees it as a response to the packet that Alice previously initiated to Bob. Now, when Alice sends her next packet to Bob, Bob's firewall sees it as a response to the packet that Bob had previously sent, and forwards it back to Bob, and you have a direct two-way connection between Alice and Bob, with each NAT thinking that it is sending only responses to packets that were first initiated from within the network.

This all relies on the external address for Alice and Bob in the phone book being accurate. However, recall that those phone entries were set up by the NAT for responses from the Server. This relies on the fact that most NATs preserve the mapping it set up to handle responses from the server for all other responses on the same client socket. Most NATs do, but not all do. Some NATs do switch things around a bit, but in predictable ways, and it's usually easy enough to modify the client to account for some of these patterns. This basic test code should work for most common NATs.

This is a simple, single threaded example, where the main thread waits for incoming packets and artificially sleeps for one second between messages. In reality, we'd want better latency than that, but the basic mechanism to traverse NATs would be largely similar to this example.

### Relationship to STUN, TURN, ICE, WebRTC etc.

There are a number of solutions to the basic problem of connecting across NATs. Pieces of this are solved by STUN, TURN, ICE and browser based support is baked into WebRTC. All of those techniques rely on some form of getting across NATs (without having to configure routers etc), and this code is intended as a simple example to demonstrate how those all work at a UDP level.

### A Sample Run

(I'm assuming the server is running on SSS.SSS.SSS.SSS on port 8080, and bob and alice are trying to connect with each other, and the XX.XX.XX.XX,YYYY represent IP addresses and portnumbers)

On the server:

```
test-puncher-server$ ./test-server -p 8080
Connection Server ready to receive on port 8080.
Make sure this is reachable from the internet.
Client from XX.XX.XX.XX : YYYY sent: alice
Adding client #0 to table of clients.
0,1,alice,XX.XX.XX.XX,YYYY
Client from XX.XX.XX.XX : YYYY sent: bob
Adding client #1 to table of clients.
0,2,alice,XX.XX.XX.XX,YYYY
1,2,bob,XX.XX.XX.XX,YYYY
Client from XX.XX.XX.XX : YYYY sent: alice
Updating client #0 in table of clients.
0,2,alice,XX.XX.XX.XX,YYYY
1,2,bob,XX.XX.XX.XX,YYYY
Client from XX.XX.XX.XX : YYYY sent: bob
Updating client #1 in table of clients.
0,2,alice,XX.XX.XX.XX,YYYY
1,2,bob,XX.XX.XX.XX,YYYY
Client from XX.XX.XX.XX : YYYY sent: alice
Updating client #1 in table of clients.
0,2,alice,XX.XX.XX.XX,YYYY
1,2,bob,XX.XX.XX.XX,YYYY
Client from XX.XX.XX.XX : YYYY sent: bob
Updating client #1 in table of clients.
0,2,alice,XX.XX.XX.XX,YYYY
1,2,bob,XX.XX.XX.XX,YYYY
```

The thing to take away from the transcript above is that alice and bob are registering with the server regularly and receiving updated copies of the phonebook (which has just two entries in this example)

At client #1, Alice:

```
alice$ ./test-client -s SSS.SSS.SSS.SSS -p 8080 -n alice -c bob
Stage Manager Server address: SSS.SSS.SSS.SSS
My Name: alice
Registering with Stage Manager.
Retrieving incoming messages
.Stage Manager sent us a Phonebook entry: 0,1,alice,XX.XX.XX.XX,YYYY
Peer: 0/1 alice@XX.XX.XX.XX,YYYY
.............................Registering with Stage Manager.
Retrieving incoming messages.
.Stage Manager sent us a Phonebook entry: 0,2,alice,XX.XX.XX.XX,YYYY
Peer: 0/2 alice@XX.XX.XX.XX,YYYY
.Stage Manager sent us a Phonebook entry: 1,2,bob,XX.XX.XX.XX,YYYY
Peer: 1/2 bob@XX.XX.XX.XX,YYYY
Located bob!
......................We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from bob to alice
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from bob to alice
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from bob to alice
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from bob to alice
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
^C
```

At Client #2, Bob:

```
$ ./test-client -s SSS.SSS.SSS.SSS -p 8080 -n bob -c alice
Stage Manager Server address: SSS.SSS.SSS.SSS
My Name: bob
Registering with Stage Manager.
Retrieving incoming messages.
.We received a peer message of len 30 from XX.XX.XX.XX,YYYY ->>
0,2,alice,XX.XX.XX.XX,YYYY

.Stage Manager sent us a Phonebook entry: 1,2,bob,XX.XX.XX.XX,YYYY
Peer: 1/2 bob@XX.XX.XX.XX:YYYY
............................Registering with Stage Manager.
Retrieving incoming messages.
.Stage Manager sent us a Phonebook entry: 0,2,alice,XX.XX.XX.XX,YYYY
Peer: 0/2 alice@XX.XX.XX.XX:YYYY
Located alice!
.Stage Manager sent us a Phonebook entry: 1,2,bob,XX.XX.XX.XX,YYYY
Peer: 1/2 bob@XX.XX.XX.XX:YYYY
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from alice to bob
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from alice to bob
.We received a peer message of len 46 from XX.XX.XX.XX,YYYY ->>
This is a direct P2P message from alice to bob
^C
```

Notice the spurious message from the server to the client which is detected as a peer message of len 30, but is really a response message from the server with a phonebook entry. It doesn't matter that we didn't catch that properly, since we'll get a proper copy when we re-register with the server soon.

The interesting bit is that the messages that say "This is a direct P2P message ..." never need to hit the server.
