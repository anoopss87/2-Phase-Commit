# 2-Phase-Commit

CS/CE/TE 6378: Project III
Design:
Server Setup: Each server will be waiting to accept connection from the initiator. The initiator will send the connection request to all the servers. Once the connection is accepted, the server will be waiting to accept connections from other servers as well as all the 7 clients and each accepted connection will be spawned in a new thread and each thread will waiting to receive messages from the corresponding socket. 
Client Setup: Each client will be waiting to accept connection from the initiator. The initiator will send the connection request to all the clients. Once the connection is accepted, each client will set up the socket connections to all the 3 servers in a separate thread and the main thread will performs 40 write operations.
Client will randomly selects one server and sends write request. The randomly selected server will buffer the message in the hash table with key being client id and broadcasts the write request to other 2 servers and wait for the agree message. The cohorts on receiving write requests from the server, buffers the message in the hash table and replies with agree message. On receiving agree message, the randomly selected server sends commit request to the leader. The leader on receiving commit request will write the message into the file and broadcasts commit message to all the cohorts. The cohorts on receiving commit message will write the message to the file and sends acknowledgement to the leader. On receiving acknowledgement from all the cohorts, the leader will send the write acknowledgement to the client.
If the randomly selected server is itself the leader then it writes the message to the file and broadcast the commit message to all the cohorts. The cohorts on receiving commit message will write the message to the file and sends acknowledgement to the randomly selected server. On receiving acknowledgement from all the cohorts, the randomly selected server will send the write acknowledgement to the client.
This process repeats for each write operation from the all 7 clients.  
Run Procedure:
Server:
Compilation: g++ server.cpp –pthread –o server
Server program requires 3 command line parameters. First parameter indicates the port number of the server, second parameter indicates server’s context.
On dc23 machine (s1):
. /server 8000 0
On dc24 machine (s2):
. /server 8001 1
On dc25 machine (s3):
. /server 8002 2
S1 is mapped to dc23.
S2 is mapped to dc24.
S3 is mapped to dc25
Client:
Compilation: g++ client.cpp –o client
Client program requires 1 common line parameter which indicates the context of the client.
For e.g.  . /client 0
. /client 1
./client 2, etc.…
Server Initiator: This has to be started only when all the 3 servers are started.
Compilation: g++ initSever.cpp –o initS
Run:  ./initS
Client Initiator: This has to be the last step i.e. only when all the 3 servers and 7 clients are started.
Compilation: g++ initClient.cpp –o initC
Run:  ./initC
Note: 
1)	In the code, s1, s2 and s3 are mapped to dc23 – 8000, dc24 – 8001 and dc25 – 8002 respectively. So the servers has to be run on dc23, dc24 and dc25 machines with the corresponding port numbers. If the server needs to be run on other machines, then the mapping has to be changed in the code (one line change).
2)	In the code, c0 to c6 are mapped to dc30 to dc36. So the clients has to run on those machines.
3)	Initiator can be run on any machine.
4)	Directories c1, c2….. c7 will have a log.txt which contains the elapsed time between write requests and write completions.
5)	Directories s1, s2 and s3 will have log.txt for message counts. 
